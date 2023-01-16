#include "plugin.hpp"


struct Chaos : Module {
	enum ParamId {
		R_PARAM,
		THRESH_PARAM,
		RESET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		R_INPUT,
		THRESH_INPUT,
		CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TENT_OUTPUT,
		NADIC_OUTPUT,
		LOGISTIC_OUTPUT,
		TENT_CV_OUTPUT,
		NADIC_CV_OUTPUT,
		LOGISTIC_CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Chaos() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(R_PARAM, 0.f, 1.f, 0.f, "");
		configParam(THRESH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "");
		configInput(R_INPUT, "");
		configInput(THRESH_INPUT, "");
		configInput(CLOCK_INPUT, "");
		configOutput(TENT_OUTPUT, "");
		configOutput(NADIC_OUTPUT, "");
		configOutput(LOGISTIC_OUTPUT, "");
		configOutput(TENT_CV_OUTPUT, "");
		configOutput(NADIC_CV_OUTPUT, "");
		configOutput(LOGISTIC_CV_OUTPUT, "");
	}
	
	dsp::SchmittTrigger trigger;
	bool triggered;
	float logisticX = 0.61834f;
	float tentX = 0.61834f;;

	void process(const ProcessArgs& args) override {
		if (trigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered ^= true;
		}

		if (!triggered) {
			return;
		}

		float r = params[R_PARAM].getValue();
		float thresh = params[THRESH_PARAM].getValue();

		logisticX = (3.f + r) * logisticX * (1 - logisticX);
		outputs[LOGISTIC_OUTPUT].setVoltage(logisticX > thresh ? 10.f : 0.f);
		outputs[LOGISTIC_CV_OUTPUT].setVoltage(logisticX);

		tentX = 2.f * r * tentX;
		tentX -= (float)floor(tentX);
		outputs[TENT_OUTPUT].setVoltage(tentX > thresh ? 10.f : 0.f);
		outputs[TENT_CV_OUTPUT].setVoltage(tentX);


		triggered = false;
	}
};


struct ChaosWidget : ModuleWidget {
	ChaosWidget(Chaos* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Chaos.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(7.0, 20.0)), module, Chaos::R_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(7.0, 35.0)), module, Chaos::THRESH_PARAM));
		addParam(createParamCentered<FlatButtonStd>(mm2px(Vec(7.0, 65.0)), module, Chaos::RESET_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 79.0)), module, Chaos::R_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 86.0)), module, Chaos::THRESH_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 93.0)), module, Chaos::CLOCK_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(19.0, 102.0)), module, Chaos::TENT_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(19.0, 109.0)), module, Chaos::NADIC_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(19.0, 116.0)), module, Chaos::LOGISTIC_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(26.0, 102.0)), module, Chaos::TENT_CV_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(26.0, 109.0)), module, Chaos::NADIC_CV_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(26.0, 116.0)), module, Chaos::LOGISTIC_CV_OUTPUT));

	}
};


Model* modelChaos = createModel<Chaos, ChaosWidget>("Chaos");