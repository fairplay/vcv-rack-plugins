#include "plugin.hpp"


struct LFSR16 : Module {
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
		NOT1_PARAM,
		NOT2_PARAM,
		SPLIT_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		GATE1_INPUT,
		GATE2_INPUT,
		GATE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		CV0_OUTPUT,
		CV1_OUTPUT,
		CV2_OUTPUT,
		CV3_OUTPUT,
		CV4_OUTPUT,
		CV5_OUTPUT,
		CV6_OUTPUT,
		CV7_OUTPUT,
		G0_OUTPUT,
		G1_OUTPUT,
		G2_OUTPUT,
		G3_OUTPUT,
		G4_OUTPUT,
		G5_OUTPUT,
		G6_OUTPUT,
		G7_OUTPUT,
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

	LFSR16() {
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

		configInput(GATE1_INPUT, "Trigger sequence 1");
		configInput(GATE2_INPUT, "Trigger sequence 1");

		configOutput(CV0_OUTPUT, "CV0");
		configOutput(CV1_OUTPUT, "CV1");
		configOutput(CV2_OUTPUT, "CV2");
		configOutput(CV3_OUTPUT, "CV3");
		configOutput(CV4_OUTPUT, "CV4");
		configOutput(CV5_OUTPUT, "CV5");
		configOutput(CV6_OUTPUT, "CV6");
		configOutput(CV7_OUTPUT, "CV7");
		configOutput(G0_OUTPUT, "Gate 0");
		configOutput(G1_OUTPUT, "Gate 1");
		configOutput(G2_OUTPUT, "Gate 2");
		configOutput(G3_OUTPUT, "Gate 3");
		configOutput(G4_OUTPUT, "Gate 4");
		configOutput(G5_OUTPUT, "Gate 5");
		configOutput(G6_OUTPUT, "Gate 6");
		configOutput(G7_OUTPUT, "Gate 7");
	}

	bool triggered1 = false;
	bool triggered2 = false;
	bool triggered = false;
	int state1 = 1;
	int state2 = 1;;
	dsp::SchmittTrigger trigger1;
	dsp::SchmittTrigger trigger2;
	dsp::SchmittTrigger trigger;
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
			lights[2 * i].setBrightness(0.9f * bit + 0.1f);
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

	void out() {
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
	}

	void process(const ProcessArgs& args) override {
		leds();
		split = (int)params[SPLIT_PARAM].getValue();

		if (trigger1.process(inputs[GATE1_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered1 ^= true;
		}
		if (trigger2.process(inputs[GATE2_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered2 ^= true;
		}

		if (triggered1) {
			state1 = lfsr(0, split, state1, params[NOT1_PARAM].getValue() > 0.f);
		}
		if (triggered2) {
			state2 = lfsr(split, MAX_LENGTH, state2, params[NOT2_PARAM].getValue() > 0.f);
		}

		if (trigger.process(inputs[GATE_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered ^= true;
		}


		bool doOut = false;
		if (inputs[GATE_INPUT].isConnected()) {
			doOut = trigger.isHigh();
		} else {
			doOut = trigger1.isHigh() || trigger2.isHigh();
		}

		if (doOut) {
			out();
		} else {
			for (int i = 0; i < 8; i++) {
				outputs[G0_OUTPUT + i].setVoltage(0.f);
			}
		}

		triggered1 = triggered2 = false;
	}

	void onReset(const ResetEvent& e) override {
		state1 = 1;
		state2 = 1;
	}
};


struct LFSR16Widget : ModuleWidget {
	LFSR16Widget(LFSR16* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LFSR16.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(15.4, 75.0)), module, LFSR16::A0_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(25.4, 75.0)), module, LFSR16::A1_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(35.4, 75.0)), module, LFSR16::A2_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(45.4, 75.0)), module, LFSR16::A3_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(15.4, 85.0)), module, LFSR16::A4_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(25.4, 85.0)), module, LFSR16::A5_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(35.4, 85.0)), module, LFSR16::A6_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(45.4, 85.0)), module, LFSR16::A7_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(15.4, 95.0)), module, LFSR16::A8_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(25.4, 95.0)), module, LFSR16::A9_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(35.4, 95.0)), module, LFSR16::A10_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(45.4, 95.0)), module, LFSR16::A11_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(15.4, 105.0)), module, LFSR16::A12_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(25.4, 105.0)), module, LFSR16::A13_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(35.4, 105.0)), module, LFSR16::A14_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(45.4, 105.0)), module, LFSR16::A15_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(5.4, 105.0)), module, LFSR16::NOT1_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(55.4, 105.0)), module, LFSR16::NOT2_PARAM));

		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(45.4, 115.0)), module, LFSR16::SPLIT_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(5.4, 85.0)), module, LFSR16::GATE1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(55.4, 85.0)), module, LFSR16::GATE2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(25.4, 115.0)), module, LFSR16::GATE_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.4, 15.0)), module, LFSR16::CV0_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(25.4, 15.0)), module, LFSR16::CV1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.4, 15.0)), module, LFSR16::CV2_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(45.4, 15.0)), module, LFSR16::CV3_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(5.4, 25.0)), module, LFSR16::CV4_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(5.4, 35.0)), module, LFSR16::CV5_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(5.4, 45.0)), module, LFSR16::CV6_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(5.4, 55.0)), module, LFSR16::CV7_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.4, 65.0)), module, LFSR16::G0_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(25.4, 65.0)), module, LFSR16::G1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.4, 65.0)), module, LFSR16::G2_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(45.4, 65.0)), module, LFSR16::G3_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(55.4, 25.0)), module, LFSR16::G4_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(55.4, 35.0)), module, LFSR16::G5_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(55.4, 45.0)), module, LFSR16::G6_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(55.4, 55.0)), module, LFSR16::G7_OUTPUT));

		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(15.4, 25.0)), module, LFSR16::X0_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(25.4, 25.0)), module, LFSR16::X1_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(35.4, 25.0)), module, LFSR16::X2_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(45.4, 25.0)), module, LFSR16::X3_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(15.4, 35.0)), module, LFSR16::X4_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(25.4, 35.0)), module, LFSR16::X5_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(35.4, 35.0)), module, LFSR16::X6_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(45.4, 35.0)), module, LFSR16::X7_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(15.4, 45.0)), module, LFSR16::X8_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(25.4, 45.0)), module, LFSR16::X9_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(35.4, 45.0)), module, LFSR16::X10_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(45.4, 45.0)), module, LFSR16::X11_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(15.4, 55.0)), module, LFSR16::X12_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(25.4, 55.0)), module, LFSR16::X13_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(35.4, 55.0)), module, LFSR16::X14_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<GreenRedLight>>(mm2px(Vec(45.4, 55.0)), module, LFSR16::X15_LIGHT));
	}
};


Model* modelLFSR16 = createModel<LFSR16, LFSR16Widget>("LFSR16");