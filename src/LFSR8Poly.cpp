#include "plugin.hpp"


struct LFSR8Poly : Module {
	enum ParamId {
		AS_PARAM,
		A0_PARAM,
		A1_PARAM,
		A2_PARAM,
		A3_PARAM,
		A4_PARAM,
		A5_PARAM,
		A6_PARAM,
		A7_PARAM,
		CV0_PARAM,
		CV1_PARAM,
		CV2_PARAM,
		CV3_PARAM,
		CV4_PARAM,
		CV5_PARAM,
		CV6_PARAM,
		CV7_PARAM,
		LEN_PARAM,
		NOT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AS_INPUT,
		LEN_INPUT,
		NOT_INPUT,
		CLOCK_INPUT,
		CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE_OUTPUT,
		CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		X0_LIGHT,
		X1_LIGHT,
		X2_LIGHT,
		X3_LIGHT,
		X4_LIGHT,
		X5_LIGHT,
		X6_LIGHT,
		X7_LIGHT,
		LIGHTS_LEN
	};

	const static int MAX_LENGTH = 8;
	const static int MAX_STATE = (1 << MAX_LENGTH) - 1;

	LFSR8Poly() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(A0_PARAM, 0.f, 1.f, 0.f, "a0");
		configParam(A1_PARAM, 0.f, 1.f, 0.f, "a1");
		configParam(A2_PARAM, 0.f, 1.f, 0.f, "a2");
		configParam(A3_PARAM, 0.f, 1.f, 0.f, "a3");
		configParam(A4_PARAM, 0.f, 1.f, 0.f, "a4");
		configParam(A5_PARAM, 0.f, 1.f, 0.f, "a5");
		configParam(A6_PARAM, 0.f, 1.f, 0.f, "a6");
		configParam(A7_PARAM, 0.f, 1.f, 0.f, "a7");

		configParam(LEN_PARAM, 1.f, 8.f, 8.f, "Sequence length");
		paramQuantities[LEN_PARAM]->snapEnabled = true;

		configParam(AS_PARAM, 0.f, (float)MAX_STATE, 0.f, "Coefficient bits base 10");
		paramQuantities[AS_PARAM]->snapEnabled = true;

		configParam(NOT_PARAM, 0.f, 1.f, 0.f, "XNOR");

		configParam(CV0_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV1_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV2_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV3_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV4_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV5_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV6_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CV7_PARAM, 0.f, 1.f, 0.5f, "");

		configInput(AS_INPUT, "LFSR coefficients");
		configInput(LEN_INPUT, "Sequence length");
		configInput(NOT_INPUT, "XNOR sequence");
		configInput(CLOCK_INPUT, "Clock");
		configInput(CV_INPUT, "State modulation");

		configOutput(GATE_OUTPUT, "");
		configOutput(CV_OUTPUT, "");
	}

	int state = 1;;
	dsp::SchmittTrigger trigger;
	int length = 8;
	int prevAs;
	bool xnor;

	void lfsr() {
		int ad = 0;

		for (int i = length - 1; i >= 0; i--) {
			ad = (ad << 1) + (int)params[A0_PARAM + i].getValue();
		}

		int newBit = __builtin_popcount(ad & state) & 1;
		state = (state << 1) | (xnor ? !newBit : newBit);
		state &= (1 << length) - 1;
	}

	void leds() {
		int bits = state;
		for(int i = 0; i < length; i++) {
			float bit = (float)(bits & 1);
			lights[X0_LIGHT + i].setBrightness(bit ? 1.0 : 0.66);
			bits >>= 1;
		}
		for(int i = length; i < 8; i++) {
			lights[i].setBrightness(0.f);
		}
	}

	void processAs(int as) {
		prevAs = as;
		for (int i = 0; i < MAX_LENGTH; i++) {
			int bit = as & 1;
			as >>= 1;
			params[A0_PARAM + i].setValue((float)bit);
		}
	}

	void process(const ProcessArgs& args) override {
		bool triggered = trigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f);
		triggered &= trigger.isHigh();

		if (inputs[NOT_INPUT].isConnected()) {
			xnor = inputs[NOT_INPUT].getVoltage() > 0.f;
			params[NOT_PARAM].setValue(xnor ? 1.0f : 0.0f);
		} else {
			xnor = params[NOT_PARAM].getValue();
		}

		if (inputs[LEN_INPUT].isConnected()) {
			length = (int)((float)(MAX_LENGTH - 1) * inputs[LEN_INPUT].getVoltage() / 10.f) + 1;
		} else {
			length = (int)params[LEN_PARAM].getValue();
		}

		int as;
		if (inputs[AS_INPUT].isConnected()) {
			as = (int)(inputs[AS_INPUT].getVoltage() / 10.f * (float)MAX_STATE);
		} else {
			as = (int)params[AS_PARAM].getValue();
		}

		if (as != prevAs) processAs(as);

		leds();

		if (!triggered) {
			return;
		}

		lfsr();

		outputs[GATE_OUTPUT].setChannels(length);
		int bits = state;
		float vo = 0.f;
		float cv0 = 1.0f;
		if (inputs[CV_INPUT].isConnected()) {
			cv0 = inputs[CV_INPUT].getVoltage() / 10.f;
		}
		for(int i = 0; i < length; i++) {
			int bit = bits & 1;
			bits >>= 1;
			outputs[GATE_OUTPUT].setVoltage(bit && trigger.isHigh() ? 10.f : 0.f, i);
			vo += (float)bit * params[CV0_PARAM + i % 8].getValue() * (float)(1 << (length - i));
		}

		outputs[CV_OUTPUT].setVoltage(10.f * cv0 * vo / (float)(1 << length));
	}

	json_t* dataToJson() override {
		json_t * rootJ = json_object();
		json_t * asJ = json_array();
		for (int i = 0; i < MAX_LENGTH; i++) {
			json_array_append_new(asJ, json_integer((int)params[A0_PARAM + i].getValue()));
		}
		json_object_set_new(rootJ, "as", asJ);
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t * asJ = json_object_get(rootJ, "as");
		if (asJ) {
			size_t i;
			json_t * a;
			json_array_foreach(asJ, i, a) {
				params[A0_PARAM + i].setValue((float)json_integer_value(a));
			}
		}
	}
};


struct LFSR8PolyWidget : ModuleWidget {
	LFSR8PolyWidget(LFSR8Poly* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LFSR8Poly.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 20.0)), module, LFSR8Poly::A0_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 24.0)), module, LFSR8Poly::A1_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 28.0)), module, LFSR8Poly::A2_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 32.0)), module, LFSR8Poly::A3_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 36.0)), module, LFSR8Poly::A4_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 40.0)), module, LFSR8Poly::A5_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 44.0)), module, LFSR8Poly::A6_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(30.0, 48.0)), module, LFSR8Poly::A7_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(14.0, 76.0)), module, LFSR8Poly::CV0_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(21.0, 76.0)), module, LFSR8Poly::CV1_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(28.0, 76.0)), module, LFSR8Poly::CV2_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(35.0, 76.0)), module, LFSR8Poly::CV3_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(14.0, 83.0)), module, LFSR8Poly::CV4_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(21.0, 83.0)), module, LFSR8Poly::CV5_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(28.0, 83.0)), module, LFSR8Poly::CV6_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(35.0, 83.0)), module, LFSR8Poly::CV7_PARAM));

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 23.0)), module, LFSR8Poly::AS_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 43.0)), module, LFSR8Poly::LEN_PARAM));
		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(33.0, 63.0)), module, LFSR8Poly::NOT_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 16.0)), module, LFSR8Poly::AS_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 36.0)), module, LFSR8Poly::LEN_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 56.0)), module, LFSR8Poly::NOT_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, LFSR8Poly::CLOCK_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 76.0)), module, LFSR8Poly::CV_INPUT));

		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(35.0, 99.0)), module, LFSR8Poly::GATE_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.0, 107.0)), module, LFSR8Poly::CV_OUTPUT));

		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 20.0)), module, LFSR8Poly::X0_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 24.0)), module, LFSR8Poly::X1_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 28.0)), module, LFSR8Poly::X2_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 32.0)), module, LFSR8Poly::X3_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 36.0)), module, LFSR8Poly::X4_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 40.0)), module, LFSR8Poly::X5_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 44.0)), module, LFSR8Poly::X6_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(35.0, 48.0)), module, LFSR8Poly::X7_LIGHT));
	}
};


Model* modelLFSR8Poly = createModel<LFSR8Poly, LFSR8PolyWidget>("LFSR8Poly");