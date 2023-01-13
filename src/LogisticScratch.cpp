#include "plugin.hpp"

struct LogisticScratch : Module {
	enum ParamId {
		FREQKNOB_PARAM,
		L1KNOB_PARAM,
		L2KNOB_PARAM,
		DXKNOB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FREQSOCKET_INPUT,
		L1SOCKET_INPUT,
		L2SOCKET_INPUT,
		DXSOCKET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTSOCKET_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	LogisticScratch() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQKNOB_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(L1KNOB_PARAM, 0.f, 1.f, 0.f, "λ1", "", 0.f, 0.25f, 0.75f);
		configParam(L2KNOB_PARAM, 0.f, 1.f, 1.f, "λ2", "", 0.f, 0.25f, 0.75f);
		configParam(DXKNOB_PARAM, 0.001f, 1.f, 0.1f, "Δx", "");
		configInput(FREQSOCKET_INPUT, "Frequency");
		configInput(L1SOCKET_INPUT, "λ1");
		configInput(L2SOCKET_INPUT, "λ2");
		configInput(DXSOCKET_INPUT, "Δx");
		configOutput(OUTSOCKET_OUTPUT);
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
		float freqVoltage = params[FREQKNOB_PARAM].getValue() / 12.f;
		if (inputs[FREQSOCKET_INPUT].isConnected()) {
			freqVoltage = inputs[FREQSOCKET_INPUT].getVoltage();
		}

		float freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(freqVoltage + 30.f) / std::pow(2.f, 30.f);
		float l1 = params[L1KNOB_PARAM].getValue();
		float l2 = params[L2KNOB_PARAM].getValue();
		float dx = params[DXKNOB_PARAM].getValue();

		if (inputs[L1SOCKET_INPUT].isConnected()) {
			l1 = inputs[L1SOCKET_INPUT].getVoltage();
			l1 = clamp(l1, 0.f, 1.f);
		}
		if (inputs[L2SOCKET_INPUT].isConnected()) {
			l2 = inputs[L2SOCKET_INPUT].getVoltage();
			l2 = clamp(l2, 0.f, 1.f);
		}
		if (inputs[DXSOCKET_INPUT].isConnected()) {
			dx = clamp(inputs[DXSOCKET_INPUT].getVoltage(), 0.001f, 1.f);
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

		outputs[OUTSOCKET_OUTPUT].setVoltage(x * 10.f - 5.f);
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

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<TsKnobLarge>(mm2px(Vec(10.16, 30.0)), module, LogisticScratch::FREQKNOB_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(3.81, 67.0)), module, LogisticScratch::L1KNOB_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(10.16, 67.0)), module, LogisticScratch::L2KNOB_PARAM));
		addParam(createParamCentered<TsKnobStd>(mm2px(Vec(16.51, 67.0)), module, LogisticScratch::DXKNOB_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(10.16, 42.0)), module, LogisticScratch::FREQSOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(3.81, 76.0)), module, LogisticScratch::L1SOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(10.16, 76.0)), module, LogisticScratch::L2SOCKET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(16.51, 76.0)), module, LogisticScratch::DXSOCKET_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.3, 106.0)), module, LogisticScratch::OUTSOCKET_OUTPUT));
	}
};


Model* modelLogisticScratch = createModel<LogisticScratch, LogisticScratchWidget>("LogisticScratch");