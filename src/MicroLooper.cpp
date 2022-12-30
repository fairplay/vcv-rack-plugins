#include "plugin.hpp"

struct MicroLooper : Module {
	enum ParamId {
		RECBTN_PARAM,
		LENGTHKNOB_PARAM,
		SCANKNOB_PARAM,
		SPEEDKNOB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		RECSOCKET_INPUT,
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
	const static int MAX_BITS = 16;
	const static int MAX_LENGTH = pow(2, MAX_BITS);

	MicroLooper() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(RECBTN_PARAM, 0.f, 1.f, 0.f, "");
		configParam(LENGTHKNOB_PARAM, (float)MIN_BITS, (float)MAX_BITS, (float)MAX_BITS, "Length", " samples", 2.f, 1.f, 0.f);
		configParam(SCANKNOB_PARAM, 0.f, (float)(MAX_LENGTH - 1), 0.f, "Scan position", "");
		configParam(SPEEDKNOB_PARAM, -5.f, 5.f, 1.f, "Speed", " samples");
		configInput(RECSOCKET_INPUT, "");
		configInput(LENGTHSOCKET_INPUT, "");
		configInput(SCANSOCKET_INPUT, "");
		configInput(SPEEDSOCKET_INPUT, "");
		configInput(INSOCKET_INPUT, "");
		configOutput(OUTSOCKET_OUTPUT, "");
	}


	bool isSmooth = false;
	bool isRecord = false;
	bool isMonitor = true;
	float recordBuffer[MAX_LENGTH];
	float playBuffer[MAX_LENGTH];
	dsp::BooleanTrigger recordTrigger;
	int recordPosition = 0;
	float playPosition = 0.f;

	void process(const ProcessArgs& args) override {

		this->isRecord = params[RECBTN_PARAM].getValue() > 0.f;

		if (recordTrigger.process(inputs[RECSOCKET_INPUT].getVoltage())) {
			this->isRecord = !this->isRecord;
			params[RECBTN_PARAM].setValue(this->isRecord ? 1.0 : 0.0);
		}


		float speed = params[SPEEDKNOB_PARAM].getValue();
		if (inputs[SPEEDSOCKET_INPUT].isConnected()) {
			speed = inputs[SPEEDSOCKET_INPUT].getVoltage();
			speed = clamp(speed, -5.f, 5.f);
		}

		float lengthRaw = params[LENGTHKNOB_PARAM].getValue();
		if (inputs[LENGTHSOCKET_INPUT].isConnected()) {
			lengthRaw = inputs[LENGTHSOCKET_INPUT].getVoltage() * (float)(MAX_BITS - MIN_BITS) * 0.1f + (float)MIN_BITS;
			lengthRaw = clamp(lengthRaw, (float)MIN_BITS, (float)MAX_BITS);
		}
		int length = pow(2, (int)lengthRaw);

		float scanPosRaw = params[SCANKNOB_PARAM].getValue();
		if (inputs[SCANSOCKET_INPUT].isConnected()) {
			scanPosRaw = inputs[SCANSOCKET_INPUT].getVoltage() * (float)(MAX_LENGTH) * 0.1f;
			scanPosRaw = clamp(scanPosRaw, 0.f, (float)(MAX_LENGTH - 1));
		}
		int scanPos = (int)scanPosRaw;

		float currentSample = 0.f;

		if (!this->isRecord) {
			int pos = (int)this->playPosition;
			float frac = this->playPosition - (float)pos;

			int realPos = pos + scanPos;
			if (realPos >= MAX_LENGTH) realPos -= MAX_LENGTH;

			currentSample =
				this->playBuffer[realPos] * (1 - frac)
				+ this->playBuffer[realPos >= MAX_LENGTH - 1 || pos >= length - 1 ? scanPos : realPos + 1] * frac;


			if (this->isSmooth) currentSample *= sin(M_PI * pos / (length - 1));

			this->playPosition += speed;

			if (this->playPosition < 0) {
				this->playPosition += (float)length;
			}
			if (this->playPosition >= (float)length) {
				this->playPosition -= (float)length;
			}
		} else {
			if (this->isMonitor) {
				currentSample = this->recordBuffer[this->recordPosition++] = inputs[INSOCKET_INPUT].getVoltage();
			}
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

		addParam(createParamCentered<TsButton>(mm2px(Vec(15.0, 24.0)), module, MicroLooper::RECBTN_PARAM));
		addParam(createParamCentered<TsKnobBig>(mm2px(Vec(15.0, 42.0)), module, MicroLooper::LENGTHKNOB_PARAM));
		addParam(createParamCentered<TsKnobBig>(mm2px(Vec(15.0, 60.0)), module, MicroLooper::SCANKNOB_PARAM));
		addParam(createParamCentered<TsKnobBig>(mm2px(Vec(15.0, 78.0)), module, MicroLooper::SPEEDKNOB_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 24.0)), module, MicroLooper::RECSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 42.0)), module, MicroLooper::LENGTHSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 60.0)), module, MicroLooper::SCANSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 78.0)), module, MicroLooper::SPEEDSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 106.0)), module, MicroLooper::INSOCKET_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(14.894, 106.0)), module, MicroLooper::OUTSOCKET_OUTPUT));
	}

	void appendContextMenu(Menu* menu) override {
		MicroLooper* module = dynamic_cast<MicroLooper*>(this->module);
		assert(module);

		menu->addChild(new MenuSeparator);

		menu->addChild(createBoolPtrMenuItem("Smooth (apply sine window)", "", &module->isSmooth));
		menu->addChild(createBoolPtrMenuItem("Monitor", "", &module->isMonitor));
	}
};


Model* modelMicroLooper = createModel<MicroLooper, MicroLooperWidget>("MicroLooper");