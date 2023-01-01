#include "plugin.hpp"


struct LFSR : Module {
	enum ParamId {
		X0_PARAM,
		X0_9_PARAM,
		X0_2_PARAM,
		X0_9_5_PARAM,
		X0_4_PARAM,
		X0_9_0_PARAM,
		X0_2_5_PARAM,
		X0_9_5_9_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		L1SOCKET_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTSOCKET_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	LFSR() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(X0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_9_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_9_5_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_4_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_9_0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_2_5_PARAM, 0.f, 1.f, 0.f, "");
		configParam(X0_9_5_9_PARAM, 0.f, 1.f, 0.f, "");
		configInput(L1SOCKET_INPUT, "");
		configOutput(OUTSOCKET_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
	}
};


struct LFSRWidget : ModuleWidget {
	LFSRWidget(LFSR* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/LFSR.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.0, 25.0)), module, LFSR::X0_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.0, 25.0)), module, LFSR::X0_9_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.0, 25.0)), module, LFSR::X0_2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.0, 25.0)), module, LFSR::X0_9_5_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15.0, 35.0)), module, LFSR::X0_4_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.0, 35.0)), module, LFSR::X0_9_0_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.0, 35.0)), module, LFSR::X0_2_5_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(45.0, 35.0)), module, LFSR::X0_9_5_9_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(3.81, 110.562)), module, LFSR::L1SOCKET_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.3, 110.0)), module, LFSR::OUTSOCKET_OUTPUT));
	}
};


Model* modelLFSR = createModel<LFSR, LFSRWidget>("LFSR");