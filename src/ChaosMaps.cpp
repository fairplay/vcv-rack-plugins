#include "plugin.hpp"

struct ChaosMaps : Module {
	enum ParamId {
		R_PARAM,
		R_MOD_PARAM,
		LINK_PARAM,
		MAP_PARAM,
		RESET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		R_INPUT,
		RESET_INPUT,
		CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		CV_OUTPUT,
		GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	ChaosMaps() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(R_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(R_MOD_PARAM, -1.f, 1.f, 0.f, "");
		configParam(LINK_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MAP_PARAM, 0.0f, 1.f, 0.f, "");
		paramQuantities[MAP_PARAM]->snapEnabled = true;
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "");
		configInput(RESET_INPUT, "");
		configInput(R_INPUT, "");
		configInput(CLOCK_INPUT, "");
		configOutput(CV_OUTPUT, "");
		configOutput(GATE_OUTPUT, "");
	}

	dsp::SchmittTrigger trigger;
	dsp::SchmittTrigger reset;
	float x = 0.61834;
	std::vector<std::string> text = {};

	enum Maps {
		LOGISTIC_MAP,
		TENT_MAP,
		MAPS_LEN
	};

	void process(const ProcessArgs& args) override {
		bool resetted = reset.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 2.f);
		resetted &= reset.isHigh();

		resetted |= params[RESET_PARAM].getValue() > 0.f;

		if (resetted) {
			onReset();
		}

		bool triggered = trigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f);
		triggered &= trigger.isHigh();

		int map = (int)params[MAP_PARAM].getValue();

		if (map == LOGISTIC_MAP) {
			text = {"LOGISTIC"};

		}

		if (map == TENT_MAP) {
			text = {"TENT"};
		}

		if (!triggered) {
			return;
		}

		float rRaw = params[R_PARAM].getValue();

		if (inputs[R_INPUT].isConnected()) {
			float mod = params[R_INPUT].getValue();
			rRaw += inputs[R_INPUT].getVoltage() * mod / 10.f;
		}
		rRaw = clamp(rRaw, 0.0, 1.0);

		float x0 = x;
		if (map == LOGISTIC_MAP) {
			float r = (rRaw + 1.0) * 2.0;
			x = r * x * (1 - x);
		}

		if (map == TENT_MAP) {
			float r = rRaw + 1.0;
			x = r * (x < 0.5f ? x : 1.0 - x);
		}

		outputs[GATE_OUTPUT].setVoltage(x < x0 ? 10.0 : 0.0);
		outputs[CV_OUTPUT].setVoltage(10.0 * x);
	}

	void onReset() override {
		x = 0.61834f;
	}
};

struct ChaosMapsWidget : ModuleWidget {
	ChaosMapsWidget(ChaosMaps* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ChaosMaps.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatSwitch>(mm2px(Vec(10.16, 21.5)), module, ChaosMaps::MAP_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 43.0)), module, ChaosMaps::R_PARAM));
		addParam(createParamCentered<FlatKnobMod>(mm2px(Vec(4.5, 43.0)), module, ChaosMaps::R_MOD_PARAM));
		addParam(createParamCentered<FlatButtonStd>(mm2px(Vec(13.0, 63.0)), module, ChaosMaps::RESET_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 36.0)), module, ChaosMaps::R_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, ChaosMaps::RESET_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 76.0)), module, ChaosMaps::CLOCK_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.0, 99.0)), module, ChaosMaps::GATE_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(15.0, 107.0)), module, ChaosMaps::CV_OUTPUT));

		FlatDisplay<ChaosMaps> * display = createWidget<FlatDisplay<ChaosMaps>>(mm2px(Vec(0, 24.5)));
		display->box.size = mm2px(Vec(20.32, 4.0));
		display->module = module;
		display->fontSize = 10;
		addChild(display);
	}
};


Model* modelChaosMaps = createModel<ChaosMaps, ChaosMapsWidget>("ChaosMaps");