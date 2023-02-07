#include "plugin.hpp"


struct LFSR16Poly : Module {
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
		AS_PARAM,
		SPLIT_PARAM,
		NOT1_PARAM,
		NOT2_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		AS_INPUT,
		SPLIT_INPUT,
		CLOCK1_INPUT,
		CLOCK2_INPUT,
		NOT1_INPUT,
		NOT2_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE1_POLY_OUTPUT,
		GATE2_POLY_OUTPUT,
		GATE_POLY_OUTPUT,
		CV1_OUTPUT,
		CV2_OUTPUT,
		CV_POLY_OUTPUT,
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
		X8_LIGHT,
		X9_LIGHT,
		X10_LIGHT,
		X11_LIGHT,
		X12_LIGHT,
		X13_LIGHT,
		X14_LIGHT,
		X15_LIGHT,
		LIGHTS_LEN
	};

	const static int MAX_LENGTH = 16;
	const static int MAX_STATE = (1 << MAX_LENGTH) - 1;

	LFSR16Poly() {
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

		configParam(AS_PARAM, 0.f, (float)MAX_STATE, 0.f, "Coefficient bits base 10");
		paramQuantities[AS_PARAM]->snapEnabled = true;

		configParam(SPLIT_PARAM, 0.f, (float)MAX_LENGTH, (float)MAX_LENGTH, "Split sequences");
		paramQuantities[SPLIT_PARAM]->snapEnabled = true;

		configInput(AS_INPUT, "LFSR coefficients");
		configInput(SPLIT_INPUT, "Split sequences");
		configInput(CLOCK1_INPUT, "Trigger sequence 1");
		configInput(CLOCK2_INPUT, "Trigger sequence 2");
		configInput(NOT1_INPUT, "XNOR sequence 1");
		configInput(NOT2_INPUT, "XNOR sequence 2");

		configOutput(GATE1_POLY_OUTPUT, "Sequence 1 gate");
		configOutput(GATE2_POLY_OUTPUT, "Sequence 2 gate");
		configOutput(GATE_POLY_OUTPUT, "Sequences matrix gate");
		configOutput(CV1_OUTPUT, "Sequence 1 CV");
		configOutput(CV2_OUTPUT, "Sequence 2 CV");
		configOutput(CV_POLY_OUTPUT, "Sequences matrix CV");
	}
	int state1 = 1;
	int state2 = 1;;
	dsp::SchmittTrigger trigger1;
	dsp::SchmittTrigger trigger2;
	int split = MAX_LENGTH;
	int prevAs = 0;

	void leds() {
		int bits = state1;
		for(int i = 0; i < split; i++) {
			float bit = (float)(bits & 1);
			lights[i].setBrightness(bit ? 1.0 : 0.66);
			bits >>= 1;
		}
		bits = state2;
		for(int i = split; i < MAX_LENGTH; i++) {
			float bit = (float)(bits & 1);
			lights[i].setBrightness(bit ? 0.0 : 0.33);
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

	void out(int bits1, int bits2) {
		outputs[CV_POLY_OUTPUT].setChannels(8);
		outputs[GATE_POLY_OUTPUT].setChannels(8);

		int mask1 = (1 << split) - 1;
		int mask2 = (1 << (MAX_LENGTH - split)) - 1;

		int bits1_0 = bits1 & mask1;
		int bits2_0 = bits2 & mask2;

		outputs[GATE1_POLY_OUTPUT].setChannels(split);
		outputs[GATE2_POLY_OUTPUT].setChannels(MAX_LENGTH - split);

		for (int i = 0; i < split; i++) {
			outputs[GATE1_POLY_OUTPUT].setVoltage(10.f * (float)(bits1_0 & 1), i);
			bits1_0 >>= 1;
		}
		for (int i = split; i < MAX_LENGTH; i++) {
			outputs[GATE2_POLY_OUTPUT].setVoltage(10.f * (float)(bits2_0 & 1), i - split);
			bits2_0 >>= 1;
		}

		outputs[CV1_OUTPUT].setVoltage(mask1 > 0 ? 10.f * (float)bits1 / (float)mask1 : 0.f);
		outputs[CV2_OUTPUT].setVoltage(mask2 > 0 ? 10.f * (float)bits2 / (float)mask2 : 0.f);

		int bits = (bits2 << split) + bits1;

		int horMask = 15;
		int verMask = 4369;

		for (int i = 0; i < 4; i++) {
			int hor = (bits & horMask) >> (i << 2);

			outputs[CV_POLY_OUTPUT].setVoltage((float)hor * 10.f / 15.f, i);
			outputs[GATE_POLY_OUTPUT].setVoltage(10.f * (float)(__builtin_popcount(hor) & 1), i);

			int ver = (bits & verMask) >> i;
			outputs[CV_POLY_OUTPUT].setVoltage(
				(float)((1 & ver) + ((1 & ver >> 4) < 1)+ ((1 & ver >> 8) < 2) + ((1 & ver >> 12) << 3)) * 10.f / 15.f,
				i + 4
			);
			outputs[GATE_POLY_OUTPUT].setVoltage(10.f * (float)(__builtin_popcount(ver) & 1), i + 4);
			verMask <<= 1;
			horMask <<= 4;
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
		split = (int)params[SPLIT_PARAM].getValue();
		if (inputs[SPLIT_INPUT].isConnected()) {
			split = (int)((float)MAX_LENGTH * inputs[SPLIT_INPUT].getVoltage() / 10.f);
		}
		if (inputs[NOT1_INPUT].isConnected()) {
			params[NOT1_PARAM].setValue(inputs[NOT1_INPUT].getVoltage() > 0.f ? 1.f : 0.f);
		}
		if (inputs[NOT2_INPUT].isConnected()) {
			params[NOT2_PARAM].setValue(inputs[NOT2_INPUT].getVoltage() > 0.f ? 1.f : 0.f);
		}

		int as;
		if (inputs[AS_INPUT].isConnected()) {
			as = (int)(inputs[AS_INPUT].getVoltage() / 10.f * (float)MAX_STATE);
		} else {
			as = (int)params[AS_PARAM].getValue();
		}
		if (as != prevAs) processAs(as);

		bool triggered1 = trigger1.process(inputs[CLOCK1_INPUT].getVoltage(), 0.1f, 2.f);
		triggered1 &= trigger1.isHigh();
		bool triggered2 = trigger2.process(inputs[CLOCK2_INPUT].getVoltage(), 0.1f, 2.f);
		triggered2 &= trigger2.isHigh();

		if (triggered1) {
			state1 = lfsr(0, split, state1, params[NOT1_PARAM].getValue() > 0.f);
		}
		if (triggered2) {
			state2 = lfsr(split, MAX_LENGTH, state2, params[NOT2_PARAM].getValue() > 0.f);
		}

		leds();

		if (triggered1 || triggered2) {
			out(state1, state2);
		}

	}

	void onReset(const ResetEvent& e) override {
		state1 = 1;
		state2 = 1;
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

struct LFSR16PolyWidget : ModuleWidget {
	LFSR16PolyWidget(LFSR16Poly* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LFSR16Poly.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(24.0, 17.0)), module, LFSR16Poly::A0_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(28.0, 17.0)), module, LFSR16Poly::A1_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(32.0, 17.0)), module, LFSR16Poly::A2_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(36.0, 17.0)), module, LFSR16Poly::A3_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(24.0, 21.0)), module, LFSR16Poly::A4_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(28.0, 21.0)), module, LFSR16Poly::A5_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(32.0, 21.0)), module, LFSR16Poly::A6_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(36.0, 21.0)), module, LFSR16Poly::A7_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(24.0, 25.0)), module, LFSR16Poly::A8_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(28.0, 25.0)), module, LFSR16Poly::A9_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(32.0, 25.0)), module, LFSR16Poly::A10_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(36.0, 25.0)), module, LFSR16Poly::A11_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(24.0, 29.0)), module, LFSR16Poly::A12_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(28.0, 29.0)), module, LFSR16Poly::A13_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(32.0, 29.0)), module, LFSR16Poly::A14_PARAM));
		addParam(createParamCentered<FlatButtonTinyPush>(mm2px(Vec(36.0, 29.0)), module, LFSR16Poly::A15_PARAM));

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 23.0)), module, LFSR16Poly::AS_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 43.0)), module, LFSR16Poly::SPLIT_PARAM));
		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(13.0, 63.0)), module, LFSR16Poly::NOT1_PARAM));
		addParam(createParamCentered<FlatButtonStdPush>(mm2px(Vec(33.0, 63.0)), module, LFSR16Poly::NOT2_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 16.0)), module, LFSR16Poly::AS_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 36.0)), module, LFSR16Poly::SPLIT_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, LFSR16Poly::NOT1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 56.0)), module, LFSR16Poly::NOT2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 76.0)), module, LFSR16Poly::CLOCK1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(26.0, 76.0)), module, LFSR16Poly::CLOCK2_INPUT));

		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(19.0, 99.0)), module, LFSR16Poly::GATE1_POLY_OUTPUT));
		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(27.0, 99.0)), module, LFSR16Poly::GATE2_POLY_OUTPUT));
		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(35.0, 99.0)), module, LFSR16Poly::GATE_POLY_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(19.0, 107.0)), module, LFSR16Poly::CV1_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(27.0, 107.0)), module, LFSR16Poly::CV2_OUTPUT));
		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(35.0, 107.0)), module, LFSR16Poly::CV_POLY_OUTPUT));

		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 35.0)), module, LFSR16Poly::X0_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(28.0, 35.0)), module, LFSR16Poly::X1_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 35.0)), module, LFSR16Poly::X2_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(36.0, 35.0)), module, LFSR16Poly::X3_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 39.0)), module, LFSR16Poly::X4_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(28.0, 39.0)), module, LFSR16Poly::X5_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 39.0)), module, LFSR16Poly::X6_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(36.0, 39.0)), module, LFSR16Poly::X7_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 43.0)), module, LFSR16Poly::X8_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(28.0, 43.0)), module, LFSR16Poly::X9_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 43.0)), module, LFSR16Poly::X10_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(36.0, 43.0)), module, LFSR16Poly::X11_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(24.0, 47.0)), module, LFSR16Poly::X12_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(28.0, 47.0)), module, LFSR16Poly::X13_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(32.0, 47.0)), module, LFSR16Poly::X14_LIGHT));
		addChild(createLightCentered<FlatLightSquareStd<BlackWhiteLight>>(mm2px(Vec(36.0, 47.0)), module, LFSR16Poly::X15_LIGHT));
	}
};


Model* modelLFSR16Poly = createModel<LFSR16Poly, LFSR16PolyWidget>("LFSR16Poly");