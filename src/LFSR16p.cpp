#include "plugin.hpp"


struct LFSR16p : Module {
	enum ParamId {
		A0_PARAM,
		A1_PARAM,
		A2_PARAM,
		A3_PARAM,
		A4_PARAM,
		A5_PARAM,
		A6_PARAM,
		A7_PARAM,
		A8_PARAM,
		A9_PARAM,
		A10_PARAM,
		A11_PARAM,
		A12_PARAM,
		A13_PARAM,
		A14_PARAM,
		A15_PARAM,
		SPLIT_PARAM,
		NOT1_PARAM,
		NOT2_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SPLIT_INPUT,
		CLOCK1_INPUT,
		CLOCK2_INPUT,
		NOT1_INPUT,
		NOT2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE1_OUTPUT,
		GATE2_OUTPUT,
		GATE_POLY_OUTPUT,
		CV1_OUTPUT,
		CV2_OUTPUT,
		CV_POLY_OUTPUT,
		OUTPUTS_LEN
	};

	enum LightId {
		ENUMS(X0_LIGHT, 2),
		ENUMS(X1_LIGHT, 2),
		ENUMS(X2_LIGHT, 2),
		ENUMS(X3_LIGHT, 2),
		ENUMS(X4_LIGHT, 2),
		ENUMS(X5_LIGHT, 2),
		ENUMS(X6_LIGHT, 2),
		ENUMS(X7_LIGHT, 2),
		ENUMS(X8_LIGHT, 2),
		ENUMS(X9_LIGHT, 2),
		ENUMS(X10_LIGHT, 2),
		ENUMS(X11_LIGHT, 2),
		ENUMS(X12_LIGHT, 2),
		ENUMS(X13_LIGHT, 2),
		ENUMS(X14_LIGHT, 2),
		ENUMS(X15_LIGHT, 2),
		LIGHTS_LEN
	};

	const static int MAX_LENGTH = 16;

	LFSR16p() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(A0_PARAM, 0.f, 1.f, 0.f, "a0");
		configParam(A1_PARAM, 0.f, 1.f, 0.f, "a1");
		configParam(A2_PARAM, 0.f, 1.f, 0.f, "a2");
		configParam(A3_PARAM, 0.f, 1.f, 0.f, "a3");
		configParam(A4_PARAM, 0.f, 1.f, 0.f, "a4");
		configParam(A5_PARAM, 0.f, 1.f, 0.f, "a5");
		configParam(A6_PARAM, 0.f, 1.f, 0.f, "a6");
		configParam(A7_PARAM, 0.f, 1.f, 0.f, "a7");
		configParam(A8_PARAM, 0.f, 1.f, 0.f, "a8");
		configParam(A9_PARAM, 0.f, 1.f, 0.f, "a9");
		configParam(A10_PARAM, 0.f, 1.f, 0.f, "a10");
		configParam(A11_PARAM, 0.f, 1.f, 0.f, "a11");
		configParam(A12_PARAM, 0.f, 1.f, 0.f, "a12");
		configParam(A13_PARAM, 0.f, 1.f, 0.f, "a13");
		configParam(A14_PARAM, 0.f, 1.f, 0.f, "a14");
		configParam(A15_PARAM, 0.f, 1.f, 0.f, "a15");

		configParam(NOT1_PARAM, 0.f, 1.f, 0.f, "Sequence 1 XNOR");
		configParam(NOT2_PARAM, 0.f, 1.f, 0.f, "Sequence 2 XNOR");

		configParam(SPLIT_PARAM, 0.f, (float)MAX_LENGTH, (float)MAX_LENGTH, "Split sequences");
		paramQuantities[SPLIT_PARAM]->snapEnabled = true;

		configInput(SPLIT_INPUT, "");
		configInput(CLOCK1_INPUT, "Trigger sequence 1");
		configInput(CLOCK2_INPUT, "Trigger sequence 1");
		configInput(NOT1_INPUT, "");
		configInput(NOT2_INPUT, "");

		configOutput(GATE1_OUTPUT, "");
		configOutput(GATE2_OUTPUT, "");
		configOutput(GATE_POLY_OUTPUT, "");
		configOutput(CV1_OUTPUT, "");
		configOutput(CV2_OUTPUT, "");
		configOutput(CV_POLY_OUTPUT, "");
	}
	bool triggered1 = false;
	bool triggered2 = false;
	int state1 = 1;
	int state2 = 1;;
	dsp::SchmittTrigger trigger1;
	dsp::SchmittTrigger trigger2;
	int split = MAX_LENGTH;

	void leds() {
		int bits = state1;
		for(int i = 0; i < split; i++) {
			float bit = (float)(bits & 1);
			lights[2 * i].setBrightness(0.f);
			lights[2 * i + 1].setBrightness(0.9f * bit + 0.1f);
			bits >>= 1;
		}
		bits = state2;
		for(int i = split; i < MAX_LENGTH; i++) {
			float bit = (float)(bits & 1);
			lights[2 * i].setBrightness(0.1f * bit + 0.9f);
			lights[2 * i + 1].setBrightness(0.f);
			bits >>= 1;
		}
	}

	int lfsr(int begin, int end, int state, bool negation) {
		int ad = 0;

		for (int i = end - 1; i >= begin; i--) {
			ad = (ad << 1) + (int)params[i].getValue();
		}

		int length = end - begin;
		int newBit = __builtin_popcount(ad & state) & 1;

		state = (state << 1) | (negation ? !newBit : newBit);
		state &= (1 << length) - 1;

		return state;
	}

	/*void out() {
		int state = (state2 << split) + state1;

		int horMask = 15;
		int verMask = 4369;

		for (int i = 0; i < 4; i++) {
			int hor = (state & horMask) >> (i << 2);

			outputs[CV4_OUTPUT + i].setVoltage((float)hor / 15.f);
			outputs[G4_OUTPUT + i].setVoltage(10.f * (float)(__builtin_popcount(hor) & 1));

			int ver = (state & verMask) >> i;
			outputs[G0_OUTPUT + i].setVoltage(10.f * (float)(__builtin_popcount(ver) & 1));
			outputs[CV0_OUTPUT + i].setVoltage(
				(float)((1 & ver) + ((1 & ver >> 4) < 1)+ ((1 & ver >> 8) < 2) + ((1 & ver >> 12) << 3)) / 15.f
			);

			verMask <<= 1;
			horMask <<= 4;
		}
		outputs[GATE1_OUT_OUTPUT].setVoltage(10.f * (float)(state1 & 1));
		outputs[GATE2_OUT_OUTPUT].setVoltage(10.f * (float)(state2 & 1));
	}*/

	void process(const ProcessArgs& args) override {
		leds();
		split = (int)params[SPLIT_PARAM].getValue();

		if (trigger1.process(inputs[CLOCK1_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered1 ^= true;
		}
		if (trigger2.process(inputs[CLOCK2_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered2 ^= true;
		}

		if (triggered1) {
			state1 = lfsr(0, split, state1, params[NOT1_PARAM].getValue() > 0.f);
		}
		if (triggered2) {
			state2 = lfsr(split, MAX_LENGTH, state2, params[NOT2_PARAM].getValue() > 0.f);
		}


		//out();

		triggered1 = triggered2 = false;
	}

	void onReset(const ResetEvent& e) override {
		state1 = 1;
		state2 = 1;
	}
};


struct LFSR16pWidget : ModuleWidget {
	LFSR16pWidget(LFSR16p* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LFSR16-poly.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(6.0, 16.0)), module, LFSR16p::A0_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(10.0, 16.0)), module, LFSR16p::A1_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(14.0, 16.0)), module, LFSR16p::A2_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(18.0, 16.0)), module, LFSR16p::A3_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(6.0, 20.0)), module, LFSR16p::A4_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(10.0, 20.0)), module, LFSR16p::A5_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(14.0, 20.0)), module, LFSR16p::A6_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(18.0, 20.0)), module, LFSR16p::A7_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(6.0, 24.0)), module, LFSR16p::A8_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(10.0, 24.0)), module, LFSR16p::A9_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(14.0, 24.0)), module, LFSR16p::A10_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(18.0, 24.0)), module, LFSR16p::A11_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(6.0, 28.0)), module, LFSR16p::A12_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(10.0, 28.0)), module, LFSR16p::A13_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(14.0, 28.0)), module, LFSR16p::A14_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(18.0, 28.0)), module, LFSR16p::A15_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(33.0, 23.0)), module, LFSR16p::SPLIT_PARAM));
		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(14.0, 83.0)), module, LFSR16p::NOT1_PARAM));
		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(33.0, 83.0)), module, LFSR16p::NOT2_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 16.0)), module, LFSR16p::SPLIT_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 68.0)), module, LFSR16p::CLOCK1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 68.0)), module, LFSR16p::CLOCK2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(7.0, 76.0)), module, LFSR16p::NOT1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 76.0)), module, LFSR16p::NOT2_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(17.0, 99.0)), module, LFSR16p::GATE1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(25.0, 99.0)), module, LFSR16p::GATE2_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(33.0, 99.0)), module, LFSR16p::GATE_POLY_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(17.0, 107.0)), module, LFSR16p::CV1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(25.0, 107.0)), module, LFSR16p::CV2_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(33.0, 107.0)), module, LFSR16p::CV_POLY_OUTPUT));

		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(8.0, 36.0)), module, LFSR16p::X0_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(16.0, 36.0)), module, LFSR16p::X1_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 36.0)), module, LFSR16p::X2_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 36.0)), module, LFSR16p::X3_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(8.0, 44.0)), module, LFSR16p::X4_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(16.0, 44.0)), module, LFSR16p::X5_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 44.0)), module, LFSR16p::X6_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 44.0)), module, LFSR16p::X7_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(8.0, 52.0)), module, LFSR16p::X8_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(16.0, 52.0)), module, LFSR16p::X9_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 52.0)), module, LFSR16p::X10_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 52.0)), module, LFSR16p::X11_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(8.0, 60.0)), module, LFSR16p::X12_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(16.0, 60.0)), module, LFSR16p::X13_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 60.0)), module, LFSR16p::X14_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 60.0)), module, LFSR16p::X15_LIGHT));
	}
};


Model* modelLFSR16p = createModel<LFSR16p, LFSR16pWidget>("LFSR16p");