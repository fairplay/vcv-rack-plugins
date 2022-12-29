#include "plugin.hpp"

struct MicroLooper : Module {
	enum ParamId {
		PLAYBTN_PARAM,
		LENGTHKNOB_PARAM,
		SCANKNOB_PARAM,
		SPEEDKNOB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		PLAYSOCKET_INPUT,
		LENGTHSOCKET_INPUT,
		SCANSOCKET_INPUT,
		SPEEDSOCKET_INPUT,
		INSOCKET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTSOCKET_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	const static int MIN_BITS = 6;
	const static int MAX_BITS = 14;
	const static int MAX_LENGTH = pow(2, MAX_BITS);
	const static int MAX_CHUNKS = pow(2, MAX_BITS - MIN_BITS) - 1;

	MicroLooper() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PLAYBTN_PARAM, 0.f, 1.f, 1.f, "");
		configParam(LENGTHKNOB_PARAM, (float)MIN_BITS, (float)MAX_BITS, (float)MAX_BITS, "Length", " samples", 2.f, 1.f, 0.f);
		configParam(SCANKNOB_PARAM, 0.f, (float)MAX_CHUNKS, 1.f, "Chunk", "", 0.f, 1.f, 1.f);
		configParam(SPEEDKNOB_PARAM, -1.f, 1.f, 1.f, "Speed", " samples");		
		configInput(PLAYSOCKET_INPUT, "");
		configInput(LENGTHSOCKET_INPUT, "");
		configInput(SCANSOCKET_INPUT, "");
		configInput(SPEEDSOCKET_INPUT, "");
		configInput(INSOCKET_INPUT, "");
		configOutput(OUTSOCKET_OUTPUT, "");
	}


	bool isPlay = false;
	float recordBuffer[MAX_LENGTH];
	float playBuffer[MAX_LENGTH];
	dsp::BooleanTrigger playTrigger;
	dsp::BooleanTrigger recordTrigger;
	int recordPosition = 0;
	float playPosition = 0.f;

	void process(const ProcessArgs& args) override {

		this->isPlay = params[PLAYBTN_PARAM].getValue() > 0.f;
		
		float speed;
		if (inputs[SPEEDSOCKET_INPUT].isConnected()) {
			speed = inputs[SPEEDSOCKET_INPUT].getVoltage() / 5.f;
			speed = clamp(speed, -1.f, 1.f);
			params[SPEEDKNOB_PARAM].setValue(speed);
		}
		speed = params[SPEEDKNOB_PARAM].getValue();

		float lengthRaw;
		if (inputs[LENGTHSOCKET_INPUT].isConnected()) {
			lengthRaw = inputs[LENGTHSOCKET_INPUT].getVoltage() * (float)(MAX_BITS - MIN_BITS) * 0.1f + (float)MIN_BITS;
			lengthRaw = clamp(lengthRaw, (float)MIN_BITS, (float)MAX_BITS);
			params[LENGTHKNOB_PARAM].setValue(lengthRaw);
		}
		lengthRaw = params[LENGTHKNOB_PARAM].getValue();
		int length = pow(2, (int)lengthRaw);

		float chunkRaw;
		if (inputs[SCANSOCKET_INPUT].isConnected()) {
			chunkRaw = inputs[SCANSOCKET_INPUT].getVoltage() * (float)(MAX_CHUNKS) * 0.1f;
			chunkRaw = clamp(chunkRaw, 0.f, (float)MAX_CHUNKS);
			params[SCANKNOB_PARAM].setValue(chunkRaw);
		}
		chunkRaw = params[SCANKNOB_PARAM].getValue();

		int chunk = (int)chunkRaw;
		float chunkFrac = chunkRaw - (float)chunk;
		chunk = (chunk * length % MAX_LENGTH) / length;

		if (playTrigger.process(inputs[PLAYSOCKET_INPUT].getVoltage())) {
			this->isPlay = !this->isPlay;
			params[PLAYBTN_PARAM].setValue(this->isPlay ? 1.0 : 0.0);
		}

		float currentSample = 0;
		if (this->isPlay) {
			int pos = (int)this->playPosition;
			float frac = this->playPosition - (float)pos;

			int realPos = pos + chunk * length;
			int realPosNext = realPos + length;
			if (realPosNext >= MAX_LENGTH) realPosNext -= MAX_LENGTH;

			float currentSample0 = 
				this->playBuffer[realPos] * (1 - frac) + 
				this->playBuffer[pos >= length - 1 ? chunk * length : realPos + 1] * frac;

			float currentSample1 = 
				this->playBuffer[realPosNext] * (1 - frac) + 
				this->playBuffer[pos >= length - 1 ? chunk * length + length : realPosNext + 1] * frac;

			currentSample = currentSample0 * (1.f - chunkFrac) + currentSample1 * chunkFrac;
			currentSample *= sin(M_PI * pos / (length - 1));

			this->playPosition += speed;

			if (this->playPosition < 0) {
				this->playPosition += (float)length;
			}
			if (this->playPosition >= (float)length) {
				this->playPosition -= (float)length;
			}
		} else {

			currentSample = this->recordBuffer[this->recordPosition++] = inputs[INSOCKET_INPUT].getVoltage();
			if (this->recordPosition >= MAX_LENGTH) {
				this->recordPosition = 0;
				for (int i = 0; i < MAX_LENGTH; i++) {
					this->playBuffer[i] = this->recordBuffer[i];
				}
			}
		}
		outputs[OUTSOCKET_OUTPUT].setVoltage(currentSample);
	}
};

struct MicroLooperWidget : ModuleWidget {
	MicroLooperWidget(MicroLooper* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/MicroLooper.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<TsButtonLatch>(mm2px(Vec(15.0, 24.0)), module, MicroLooper::PLAYBTN_PARAM));
		addParam(createParamCentered<TsKnobBig>(mm2px(Vec(15.0, 42.0)), module, MicroLooper::LENGTHKNOB_PARAM));
		addParam(createParamCentered<TsKnobBig>(mm2px(Vec(15.0, 60.0)), module, MicroLooper::SCANKNOB_PARAM));
		addParam(createParamCentered<TsKnobBig>(mm2px(Vec(15.0, 78.0)), module, MicroLooper::SPEEDKNOB_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 24.0)), module, MicroLooper::PLAYSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 42.0)), module, MicroLooper::LENGTHSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 60.0)), module, MicroLooper::SCANSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 78.0)), module, MicroLooper::SPEEDSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 106.0)), module, MicroLooper::INSOCKET_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(14.894, 106.0)), module, MicroLooper::OUTSOCKET_OUTPUT));
	}
};


Model* modelMicroLooper = createModel<MicroLooper, MicroLooperWidget>("MicroLooper");