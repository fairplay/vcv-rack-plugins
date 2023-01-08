#include "plugin.hpp"

struct MicroLooper : Module {
	enum ParamId {
		RECBTN_PARAM,
		LENGTHKNOB_PARAM,
		SCANKNOB_PARAM,
		SPEEDKNOB_PARAM,
		WETKNOB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RECSOCKET_INPUT,
		LENGTHSOCKET_INPUT,
		SCANSOCKET_INPUT,
		SPEEDSOCKET_INPUT,
		WETSOCKET_INPUT,
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

	const static int MIN_BITS = 8;
	const static int MAX_BITS = 15;
	const static int MAX_LENGTH = 1 << MAX_BITS;
	const static int MAX_CHUNKS = (1 << (MAX_BITS - MIN_BITS)) - 1;

	MicroLooper() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RECBTN_PARAM, 0.f, 1.f, 0.f, "Recording");
		configParam(LENGTHKNOB_PARAM, (float)MIN_BITS, (float)MAX_BITS, (float)MAX_BITS, "Length", "", 2.f, 1.f, 0.f);
		configParam(SCANKNOB_PARAM, 0.f, 1.f, 1.f, "Chunk", "");
		configParam(SPEEDKNOB_PARAM, -5.f, 5.f, 1.f, "Speed", " samples");
		configParam(WETKNOB_PARAM, 0.f, 1.f, 1.f, "Dry/Wet", "", 0.f, 100.f, 0.f);
		configInput(RECSOCKET_INPUT, "Record");
		configInput(LENGTHSOCKET_INPUT, "Length");
		configInput(SCANSOCKET_INPUT, "Scan position");
		configInput(SPEEDSOCKET_INPUT, "Speed");
		configInput(WETSOCKET_INPUT, "Dry/Wet");
		configInput(INSOCKET_INPUT, "");
		configOutput(OUTSOCKET_OUTPUT, "");

		paramQuantities[LENGTHKNOB_PARAM]->snapEnabled = true;
	}

	bool isRecord = false;
	float recordBuffer[MAX_LENGTH] = {0.f};
	float playBuffer[MAX_LENGTH] = {0.f};
	int recordPosition = 0;
	float playPosition = 0.f;
	int length0 = MAX_LENGTH;

	void process(const ProcessArgs& args) override {
		this->isRecord = params[RECBTN_PARAM].getValue() > 0.f;
		if (inputs[RECSOCKET_INPUT].isConnected()) {
			this->isRecord = inputs[RECSOCKET_INPUT].getVoltage() > 0.f;
			params[RECBTN_PARAM].setValue(this->isRecord ? 1.0f : 0.f);
		}

		float speed = params[SPEEDKNOB_PARAM].getValue();
		if (inputs[SPEEDSOCKET_INPUT].isConnected()) {
			speed = inputs[SPEEDSOCKET_INPUT].getVoltage();
			speed = clamp(speed, -5.f, 5.f);
		}

		float lengthRaw = params[LENGTHKNOB_PARAM].getValue();
		if (inputs[LENGTHSOCKET_INPUT].isConnected()) {
			float lengthVoltage = clamp(inputs[LENGTHSOCKET_INPUT].getVoltage(), 0.f, 1.f);
			lengthRaw =  lengthVoltage * (float)(MAX_BITS - MIN_BITS) + (float)MIN_BITS;
		}
		int length = 1 << ((int)lengthRaw - 1);
		if (this->length0 != length) {
			this->playPosition = 0;
		}
		this->length0 = length;

		float chunkRaw = params[SCANKNOB_PARAM].getValue();
		if (inputs[SCANSOCKET_INPUT].isConnected()) {
			chunkRaw = inputs[SCANSOCKET_INPUT].getVoltage();
		}
		chunkRaw = clamp(chunkRaw, 0.f, 1.0f);
		chunkRaw *= ((float)MAX_LENGTH / (float)length - 1.f);
		int chunk = (int)chunkRaw;
		float chunkFrac = chunkRaw - (float)chunk;

		float wet = params[WETKNOB_PARAM].getValue();
		if (inputs[WETSOCKET_INPUT].isConnected()) {
			wet = inputs[WETSOCKET_INPUT].getVoltage();
			wet = clamp(wet, 0.f, 1.f);
		}

		float currentSample = 0.f;

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

		this->playPosition += speed;

		if (this->playPosition < 0) {
			this->playPosition += (float)length;
		}
		if (this->playPosition >= (float)length) {
			this->playPosition -= (float)length;
		}

		this->recordBuffer[this->recordPosition++] = inputs[INSOCKET_INPUT].getVoltage();

		if (this->recordPosition >= MAX_LENGTH) {
			if (this->isRecord) {
				for (int i = 0; i < MAX_LENGTH; i++) {
					this->playBuffer[i] = this->recordBuffer[i];
				}
			}
			this->recordPosition = 0;
		}

		outputs[OUTSOCKET_OUTPUT].setVoltage(currentSample * wet + this->recordBuffer[this->recordPosition] * (1.0f - wet));
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

		addParam(createParamCentered<TsButtonStd>(mm2px(Vec(14.5, 23.5)), module, MicroLooper::RECBTN_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(14.5, 39.5)), module, MicroLooper::LENGTHKNOB_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(14.5, 55.5)), module, MicroLooper::SCANKNOB_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(14.5, 71.5)), module, MicroLooper::SPEEDKNOB_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(14.5, 87.5)), module, MicroLooper::WETKNOB_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(5.5, 23.5)), module, MicroLooper::RECSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.5, 39.5)), module, MicroLooper::LENGTHSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.5, 55.5)), module, MicroLooper::SCANSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.5, 71.5)), module, MicroLooper::SPEEDSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.5, 87.5)), module, MicroLooper::WETSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 106.0)), module, MicroLooper::INSOCKET_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(14.894, 106.0)), module, MicroLooper::OUTSOCKET_OUTPUT));
	}
};


Model* modelMicroLooper = createModel<MicroLooper, MicroLooperWidget>("MicroLooper");