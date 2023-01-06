#include "plugin.hpp"

struct LFSR8 : Module {
	enum ParamId {
		A0_PARAM,
		A1_PARAM,
		A2_PARAM,
		A3_PARAM,
		A4_PARAM,
		A5_PARAM,
		A6_PARAM,
		A7_PARAM,
		NOT_PARAM,
		LEN_PARAM,
		CVO0_PARAM,
		CVO1_PARAM,
		CVO2_PARAM,
		CVO3_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		GATE_INPUT,
		I0_INPUT,
		I1_INPUT,
		I2_INPUT,
		I3_INPUT,
		I4_INPUT,
		I5_INPUT,
		I6_INPUT,
		I7_INPUT,
		INOT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GO0_OUTPUT,
		GO1_OUTPUT,
		GO2_OUTPUT,
		GO3_OUTPUT,
		GO4_OUTPUT,
		GO5_OUTPUT,
		GO6_OUTPUT,
		GO7_OUTPUT,
		VO0_OUTPUT,
		VO1_OUTPUT,
		VO2_OUTPUT,
		VO3_OUTPUT,
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

	LFSR8() {
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

		configParam(NOT_PARAM, 0.f, 1.f, 0.f, "XNOR");

		configParam(CVO0_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CVO1_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CVO2_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(CVO3_PARAM, 0.f, 1.f, 0.5f, "");

		configInput(GATE_INPUT, "Clock");
		configInput(I0_INPUT, "a0");
		configInput(I1_INPUT, "a1");
		configInput(I2_INPUT, "a2");
		configInput(I3_INPUT, "a3");
		configInput(I4_INPUT, "a4");
		configInput(I5_INPUT, "a5");
		configInput(I6_INPUT, "a6");
		configInput(I7_INPUT, "a7");
		configInput(INOT_INPUT, "XNOR");

		configOutput(GO0_OUTPUT, "x0");
		configOutput(GO1_OUTPUT, "x1");
		configOutput(GO2_OUTPUT, "x2");
		configOutput(GO3_OUTPUT, "x3");
		configOutput(GO4_OUTPUT, "x4");
		configOutput(GO5_OUTPUT, "x5");
		configOutput(GO6_OUTPUT, "x6");
		configOutput(GO7_OUTPUT, "x7");

		configOutput(VO0_OUTPUT, "CV0");
		configOutput(VO1_OUTPUT, "CV1");
		configOutput(VO2_OUTPUT, "CV2");
		configOutput(VO3_OUTPUT, "CV3");
	}

	bool triggered;
	__UINT8_TYPE__ state = 1;;
	dsp::SchmittTrigger trigger;
	int length = 8;

	void lfsr() {
		__UINT8_TYPE__ ad = 0;

		for (int i = length - 1; i >= 0; i--) {
			ad = (ad << 1) + (__UINT8_TYPE__)params[i].getValue();
		}

		__UINT8_TYPE__ newBit = __builtin_popcount(ad & state) & 1;
		state = (state << 1) | (params[NOT_PARAM].getValue() > 0.f ? !newBit : newBit);
		state &= (1 << length) - 1;
	}

	void leds() {
		__UINT8_TYPE__ bits = state;
		for(int i = 0; i < length; i++) {
			float bit = (float)(bits & 1);
			lights[i].setBrightness(0.9f * bit + 0.1f);
			bits >>= 1;
		}
		for(int i = length; i < 8; i++) {
			lights[i].setBrightness(0.f);
		}
	}

	void process(const ProcessArgs& args) override {
		leds();

		length = (int)params[LEN_PARAM].getValue();

		for (int i = I0_INPUT; i <= I7_INPUT; i++) {
			if (inputs[i].isConnected()) {
				params[i - 1].setValue(inputs[i].getVoltage() > 0.f ? 1.f : 0.f);
			}
		}

		if (inputs[INOT_INPUT].isConnected()) {
			params[NOT_PARAM].setValue(inputs[INOT_INPUT].getVoltage() > 0.f ? 1.f : 0.f);
		}

		if (trigger.process(inputs[GATE_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered ^= true;
		}

		if (triggered) {
			lfsr();
		}

		float vo0 = 0.f, vo1 = 0.f, vo2 = 0.f, vo3 = 0.f;

		for(int i = 0; i < 8; i++) {
			int gateOut = GO0_OUTPUT + i;
			int bit = (state & (1 << gateOut)) >> gateOut;

			float go = 0.f;
			if (trigger.isHigh()) {
				go = (float)(bit * 10);
			}
			outputs[gateOut].setVoltage(go);

			vo0 += (float)bit * params[(0 + i) % 4 + CVO0_PARAM].getValue();
			vo1 += (float)bit * params[(1 + i) % 4 + CVO0_PARAM].getValue();
			vo2 += (float)bit * params[(2 + i) % 4 + CVO0_PARAM].getValue();
			vo3 += (float)bit * params[(3 + i) % 4 + CVO0_PARAM].getValue();
		}

		if (trigger.isHigh()) {
			outputs[VO0_OUTPUT].setVoltage(vo0);
			outputs[VO1_OUTPUT].setVoltage(vo1);
			outputs[VO2_OUTPUT].setVoltage(vo2);
			outputs[VO3_OUTPUT].setVoltage(vo3);
		}

		triggered = false;
	}

	void onReset(const ResetEvent& e) override {
		state = 1;
	}
};

struct LFSR8Widget : ModuleWidget {
	LFSR8Widget(LFSR8* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LFSR8.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 25.0)), module, LFSR8::A0_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 35.0)), module, LFSR8::A1_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 45.0)), module, LFSR8::A2_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 55.0)), module, LFSR8::A3_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 65.0)), module, LFSR8::A4_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 75.0)), module, LFSR8::A5_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 85.0)), module, LFSR8::A6_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 95.0)), module, LFSR8::A7_PARAM));
		addParam(createParamCentered<TsButtonStdPush>(mm2px(Vec(20.0, 105.0)), module, LFSR8::NOT_PARAM));

		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(10.0, 105.0)), module, LFSR8::LEN_PARAM));

		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(50.0, 25.0)), module, LFSR8::CVO0_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(50.0, 35.0)), module, LFSR8::CVO1_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(50.0, 45.0)), module, LFSR8::CVO2_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(50.0, 55.0)), module, LFSR8::CVO3_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 25.0)), module, LFSR8::I0_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 35.0)), module, LFSR8::I1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 45.0)), module, LFSR8::I2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 55.0)), module, LFSR8::I3_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 65.0)), module, LFSR8::I4_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 75.0)), module, LFSR8::I5_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 85.0)), module, LFSR8::I6_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 95.0)), module, LFSR8::I7_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(30.0, 105.0)), module, LFSR8::INOT_INPUT));

		addInput(createInputCentered<Inlet>(mm2px(Vec(10.0, 15.0)), module, LFSR8::GATE_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 25.0)), module, LFSR8::GO0_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 35.0)), module, LFSR8::GO1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 45.0)), module, LFSR8::GO2_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 55.0)), module, LFSR8::GO3_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 65.0)), module, LFSR8::GO4_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 75.0)), module, LFSR8::GO5_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 85.0)), module, LFSR8::GO6_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(40.0, 95.0)), module, LFSR8::GO7_OUTPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(50.0, 65.0)), module, LFSR8::VO0_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(50.0, 75.0)), module, LFSR8::VO1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(50.0, 85.0)), module, LFSR8::VO2_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(50.0, 95.0)), module, LFSR8::VO3_OUTPUT));

		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 25.0)), module, LFSR8::X0_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 35.0)), module, LFSR8::X1_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 45.0)), module, LFSR8::X2_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 55.0)), module, LFSR8::X3_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 65.0)), module, LFSR8::X4_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 75.0)), module, LFSR8::X5_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 85.0)), module, LFSR8::X6_LIGHT));
		addChild(createLightCentered<TsLightStd<RedLight>>(mm2px(Vec(10.0, 95.0)), module, LFSR8::X7_LIGHT));
	}
};


Model* modelLFSR8 = createModel<LFSR8, LFSR8Widget>("LFSR8");
