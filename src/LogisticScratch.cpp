#include "plugin.hpp"

struct LogisticScratch : Module {
	enum ParamId {
		FREQ_PARAM,
		FM_PARAM,
		L1_PARAM,
		L1_MOD_PARAM,
		L2_PARAM,
		L2_MOD_PARAM,
		DX_PARAM,
		DX_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FREQ_INPUT,
		L1_INPUT,
		L2_INPUT,
		DX_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	LogisticScratch() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(FM_PARAM, -1.f, 1.f, 1.f, "FM amount", "");
		configParam(L1_PARAM, 0.f, 1.f, 0.f, "λ1", "", 0.f, 0.25f, 0.75f);
		configParam(L1_MOD_PARAM, -1.f, 1.f, 0.f, "λ1 modulation amount", "");
		configParam(L2_PARAM, 0.f, 1.f, 1.f, "λ2", "", 0.f, 0.25f, 0.75f);
		configParam(L2_MOD_PARAM, -1.f, 1.f, 0.f, "λ2 modulation amount", "");
		configParam(DX_PARAM, 0.001f, 1.f, 0.1f, "Δx", "");
		configParam(DX_MOD_PARAM, -1.f, 1.f, 0.f, "Δx modulation amount", "");
		configInput(FREQ_INPUT, "Frequency");
		configInput(L1_INPUT, "λ1 modulation");
		configInput(L2_INPUT, "λ2 modulation");
		configInput(DX_INPUT, "Δx modulation");
		configOutput(OUT_OUTPUT, "");
	}

	enum Stage {
		SLIP,
		STICK,
		STAGES_LEN
	};

	Stage stage = SLIP;
	float x = 0.618f;
	float xn = 0.f;
	float l1, l2;
	float l = 0.f;

	void process(const ProcessArgs& args) override {
		float freqVoltage = params[FREQ_PARAM].getValue() / 12.0;
		if (inputs[FREQ_INPUT].isConnected()) {
			float fm = params[FM_PARAM].getValue();
			freqVoltage += inputs[FREQ_INPUT].getVoltage() * fm;
		}

		float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(freqVoltage + 30.f) / std::pow(2.f, 30.f);
		float l1 = params[L1_PARAM].getValue();
		float l2 = params[L2_PARAM].getValue();
		float dx = params[DX_PARAM].getValue();

		if (inputs[L1_INPUT].isConnected()) {
			float mod = params[L1_MOD_PARAM].getValue();
			l1 += inputs[L1_INPUT].getVoltage() / 10.f * mod;
			l1 = clamp(l1, 0.f, 1.f);
		}
		if (inputs[L2_INPUT].isConnected()) {
			float mod = params[L2_MOD_PARAM].getValue();
			l2 += inputs[L2_INPUT].getVoltage() / 10.f * mod;
			l2 = clamp(l2, 0.f, 1.f);
		}
		if (inputs[DX_INPUT].isConnected()) {
			float mod = params[DX_MOD_PARAM].getValue();
			dx += inputs[DX_INPUT].getVoltage() / 10.f * mod;
			dx = clamp(dx, 0.001f, 1.f);
		}

		l1 = l1 * 0.25f + 0.75f;
		l2 = l2 * 0.25f + 0.75f;

		if (l < 0.75f) {
			l = l1;
		}

		float step = (l2 - l1) * freq / APP->engine->getSampleRate();

		l += step;

		if (l1 <= l2) {
			l = l > l2 ? l1 : l;
			l = l < l1 ? l2 : l;
		} else {
			l = l > l1 ? l2 : l;
			l = l < l2 ? l1 : l;
		}

		if (stage == SLIP) {
			xn = 4.f * l * x * (1.f - x);
		}

		x -= dx;

		if (x > xn) {
			stage = STICK;
		} else {
			x = xn;
			stage = SLIP;
		}

		outputs[OUT_OUTPUT].setVoltage(x * 10.f - 5.f);
	}

	void onReset(const ResetEvent& e) override {
		x = 0.618f;
		xn = 0.f;
		l = 0.f;
	}
};


struct LogisticScratchWidget : ModuleWidget {
	LogisticScratchWidget(LogisticScratch* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LogisticScratch.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 23.0)), module, LogisticScratch::FREQ_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 24.0)), module, LogisticScratch::FM_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 43.0)), module, LogisticScratch::L1_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 44.0)), module, LogisticScratch::L1_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 63.0)), module, LogisticScratch::L2_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 64.0)), module, LogisticScratch::L2_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 83.0)), module, LogisticScratch::DX_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 84.0)), module, LogisticScratch::DX_MOD_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 16.0)), module, LogisticScratch::FREQ_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 36.0)), module, LogisticScratch::L1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, LogisticScratch::L2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 76.0)), module, LogisticScratch::DX_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.0, 99.0)), module, LogisticScratch::OUT_OUTPUT));
	}
};


Model* modelLogisticScratch = createModel<LogisticScratch, LogisticScratchWidget>("LogisticScratch");