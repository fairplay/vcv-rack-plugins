#include "plugin.hpp"

struct LogisticScratch : Module {
	enum ParamId {
		FREQ_PARAM,
		L1_PARAM,
		L2_PARAM,
		DX_PARAM,
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
		configParam(L1_PARAM, 0.f, 1.f, 0.f, "λ1", "", 0.f, 0.25f, 0.75f);
		configParam(L2_PARAM, 0.f, 1.f, 1.f, "λ2", "", 0.f, 0.25f, 0.75f);
		configParam(DX_PARAM, 0.001f, 1.f, 0.1f, "Δx", "");
		configInput(FREQ_INPUT, "Frequency");
		configInput(L1_INPUT, "λ1");
		configInput(L2_INPUT, "λ2");
		configInput(DX_INPUT, "Δx");
		configOutput(OUT_OUTPUT);
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
		float freqVoltage = params[FREQ_PARAM].getValue() / 12.f;
		if (inputs[FREQ_INPUT].isConnected()) {
			freqVoltage = inputs[FREQ_INPUT].getVoltage();
		}

		float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(freqVoltage + 30.f) / std::pow(2.f, 30.f);
		float l1 = params[L1_PARAM].getValue();
		float l2 = params[L2_PARAM].getValue();
		float dx = params[DX_PARAM].getValue();

		if (inputs[L1_INPUT].isConnected()) {
			l1 = inputs[L1_INPUT].getVoltage() / 10.f;
			l1 = clamp(l1, 0.f, 1.f);
		}
		if (inputs[L2_INPUT].isConnected()) {
			l2 = inputs[L2_INPUT].getVoltage() / 10.f;
			l2 = clamp(l2, 0.f, 1.f);
		}
		if (inputs[DX_INPUT].isConnected()) {
			dx = inputs[DX_INPUT].getVoltage() / 10.f;
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

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(12.0, 23.0)), module, LogisticScratch::FREQ_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(12.0, 43.0)), module, LogisticScratch::L1_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(12.0, 63.0)), module, LogisticScratch::L2_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(12.0, 83.0)), module, LogisticScratch::DX_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 16.0)), module, LogisticScratch::FREQ_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 36.0)), module, LogisticScratch::L1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 56.0)), module, LogisticScratch::L2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(5.0, 76.0)), module, LogisticScratch::DX_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.0, 99.0)), module, LogisticScratch::OUT_OUTPUT));
	}
};


Model* modelLogisticScratch = createModel<LogisticScratch, LogisticScratchWidget>("LogisticScratch");