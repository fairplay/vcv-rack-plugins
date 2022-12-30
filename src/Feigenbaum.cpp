#include "plugin.hpp"


struct Feigenbaum : Module {
	enum ParamId {
		R_PARAM,
		DX_PARAM,
		R_MOD_PARAM,
		DX_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DX_INPUT,
		R_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};
	
	enum Stage {
		SLIP,
		STICK,
		STAGES_LEN
	};

	Stage stage = SLIP;
	float x = 0.618f;
	float xn = 0.f;

	Feigenbaum() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(R_PARAM, 0.5f, 1.f, 0.75f, "");
		configParam(DX_PARAM, 0.001f, 1.f, 0.001f, "");
		configParam(R_MOD_PARAM, 0.f, 22050.f, 440.f, "");
		configParam(DX_MOD_PARAM, 0.f, 1.f, 0.f, "");
		configInput(R_INPUT, "");
		configInput(DX_INPUT, "");
		configOutput(OUTPUT, "");
	}

	float r;
	float r0;

	void process(const ProcessArgs& args) override {
		float r1 = params[R_PARAM].getValue();
		if (r1 != r0) {
			r = r0 = r1;
		}

		float dx = params[DX_PARAM].getValue();
		float dr = params[R_MOD_PARAM].getValue() / APP->engine->getSampleRate();

		float r_inp = 1.f;
		if (inputs[R_INPUT].isConnected()) {
			r_inp = inputs[R_INPUT].getVoltage() / 10.f;
		}

		float dx_inp = 1.f;
		if (inputs[DX_INPUT].isConnected()) {
			dx_inp = inputs[DX_INPUT].getVoltage() / 10.f;
		}

		r += dr;

		r = r > 1.f ? r0 : r;
		r = r < r0 ? 1.f : r;

		/*dx += dx_inp;
		dx = dx > 1.f ? 0.f : dx;
		dx = dx < 0.f ? 1.f : dx;*/
		
		if (stage == SLIP) {
			xn = 4.f * r * x * (1.f - x);
		}

		x -= dx;

		if (x > xn) {
			stage = STICK;
		} else {
			x = xn;
			stage = SLIP;	
		}
		float x0 = (2.0f * x - 1.0f) / r;

		outputs[OUTPUT].setVoltage(x);
	}
};


struct FeigenbaumWidget : ModuleWidget {
	FeigenbaumWidget(Feigenbaum* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Feigenbaum.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.0, 46.0)), module, Feigenbaum::R_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.0, 46.0)), module, Feigenbaum::DX_PARAM));
		
		addParam(createParamCentered<Trimpot>(mm2px(Vec(7.0, 61.0)), module, Feigenbaum::R_MOD_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(21.0, 61.0)), module, Feigenbaum::DX_MOD_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.0, 76.0)), module, Feigenbaum::R_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.0, 76.0)), module, Feigenbaum::DX_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(16.0, 110.0)), module, Feigenbaum::OUTPUT));
	}
};


Model* modelFeigenbaum = createModel<Feigenbaum, FeigenbaumWidget>("Feigenbaum");
