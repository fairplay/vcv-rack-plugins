#include "plugin.hpp"

struct Chaos : Module {
	enum ParamId {
		R1_PARAM,
		R2_PARAM,
		LINK_PARAM,
		MAP_PARAM,
		RESET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		R1_INPUT,
		R2_INPUT,
		CLOCK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MAP_CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Chaos() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(R1_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(R2_PARAM, 0.f, 1.f, 0.5f, "");
		configParam(LINK_PARAM, 0.f, 1.f, 0.f, "");
		configParam(MAP_PARAM, 0.0f, 2.f, 0.f, "");
		paramQuantities[MAP_PARAM]->snapEnabled = true;
		configParam(RESET_PARAM, 0.f, 1.f, 0.f, "");
		configInput(R1_INPUT, "");
		configInput(R2_INPUT, "");
		configInput(CLOCK_INPUT, "");
		configOutput(MAP_CV_OUTPUT, "");
	}

	dsp::SchmittTrigger trigger;
	bool triggered;
	float logisticX = 0.61834f;
	float nadicX = 0.61834f;
	float tentX = 0.61834f;
	std::string text = "";
	bool rFirst = true;
	float r1_0, r2_0;
	bool link = false;

	enum Maps {
		LOGISTIC_MAP,
		NADIC_MAP,
		TENT_MAP,
		MAPS_LEN
	};

	void process(const ProcessArgs& args) override {
		if (params[RESET_PARAM].getValue() > 0) {
			onReset();
		}

		if (trigger.process(inputs[CLOCK_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered ^= true;
		}

		if (!triggered) {
			return;
		}

		float r;
		float r1 = params[R1_PARAM].getValue();
		float r2 = params[R2_PARAM].getValue();

		link = false;
		if (params[LINK_PARAM].getValue() > 0.f) {
			if (!link) {
				params[R2_PARAM].setValue(r1);
			}
			link = true;
		}

		if (r1 != r1_0 && link) {
			params[R2_PARAM].setValue(r1);
			r2 = r1;
		}
		if (r2 != r2_0 && link) {
			params[R1_PARAM].setValue(r2);
			r1 = r2;
		}

		r1_0 = r1;
		r2_0 = r2;

		if (inputs[R1_INPUT].isConnected()) {
			r1 = inputs[R1_INPUT].getVoltage() / 10.f;
		}
		if (inputs[R2_INPUT].isConnected()) {
			r2 = inputs[R2_INPUT].getVoltage() / 10.f;
		}

		if (rFirst) {
			r = r1;
		} else {
			r = r2;
		}
		rFirst ^= true;

		int map = (int)params[MAP_PARAM].getValue();

		float result = 0.f;
		if (map == LOGISTIC_MAP) {
			text = "Logistic";
			result = logisticX = (3.f + r) * logisticX * (1 - logisticX);
		}
		if (map == NADIC_MAP) {
			text = "n-Adic";
			nadicX = (1.f + r) * nadicX;
			result = nadicX -= (float)floor(nadicX);
		}
		if (map == TENT_MAP) {
			text = "Tent";
			result = tentX = (1.f + r) * (tentX < 0.5f ? tentX : 1.f - tentX);
		}

		outputs[MAP_CV_OUTPUT].setVoltage(10.f * result);

		triggered = false;
	}

	void onReset() override {
		logisticX = 0.61834f;
		nadicX = 0.61834f;
		tentX = 0.61834f;
	}
};

struct Display : LedDisplay {
	Chaos* module;
	std::string text;

	std::string fontPath;
	std::shared_ptr<Font> font;

	Display() {
		fontPath = asset::plugin(pluginInstance, "res/fonts/trim.ttf");
		font = APP->window->loadFont(fontPath);
	}

	void step() override {
		if (!module) {
			return;
		}
		text = module->text;
	}

	void draw(const DrawArgs& args) override {
		if (!font) {
			return;
		}

		nvgSave(args.vg);
		math::Rect r = box.zeroPos().shrink(mm2px(math::Vec(1, 0)));;
		nvgBeginPath(args.vg);
		nvgRect(args.vg, RECT_ARGS(r));
		nvgFillColor(args.vg, nvgRGBA(0x4c, 0xda, 0xf8, 0x10));
		nvgFill(args.vg);

		nvgFontFaceId(args.vg, font->handle);
		nvgTextLetterSpacing(args.vg, 1.0);
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFillColor(args.vg, nvgRGBA(0x4c, 0xda, 0xf8, 0xff));
		nvgFontSize(args.vg, 14);
		nvgText(args.vg, 30.0, mm2px(2), text.c_str(), NULL);
		nvgRestore(args.vg);
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

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(10.16, 20.0)), module, Chaos::R1_PARAM));
		addParam(createParamCentered<FlatButtonTiny>(mm2px(Vec(10.16, 29.6)), module, Chaos::LINK_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(10.16, 40.0)), module, Chaos::R2_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(10.16, 60.0)), module, Chaos::MAP_PARAM));
		addParam(createParamCentered<FlatButtonSmall>(mm2px(Vec(10.16, 84.0)), module, Chaos::RESET_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(4.412, 94.0)), module, Chaos::R1_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(4.412, 101.0)), module, Chaos::R2_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(4.412, 108.0)), module, Chaos::CLOCK_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(13.82, 118.25)), module, Chaos::MAP_CV_OUTPUT));

		Display* display = createWidget<Display>(mm2px(Vec(0.0, 66.0)));
		display->box.size = mm2px(Vec(20.32, 5.0));
		display->module = module;
		addChild(display);
	}
};


Model* modelChaos = createModel<Chaos, ChaosWidget>("Chaos");