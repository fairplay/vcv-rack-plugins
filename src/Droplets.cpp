#include "plugin.hpp"

struct Droplets : Module {
	enum ParamId {
		FLOW_PARAM,
		FLOW_MOD_PARAM,
		VISC_PARAM,
		VISC_MOD_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		FLOW_INPUT,
		VISC_INPUT,
		TICK_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DROP_OUTPUT,
		CV_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		L0_LIGHT,
		L1_LIGHT,
		L2_LIGHT,
		L3_LIGHT,
		L4_LIGHT,
		L5_LIGHT,
		L6_LIGHT,
		L7_LIGHT,
		LIGHTS_LEN
	};

	const float MAX_FLOW = 1.0;
	const float MAX_VISC = 10.0;

	struct Cell {
		bool drop;
		float volume;
		Cell() {
			drop = false;
			volume = 0.0;
		}
	};

	struct Pipe {
		const static int MAX_CELLS = 8;
		Cell * cells[MAX_CELLS];
		float maxVolume = 10.0;

		Pipe() {
			for (int i = 0; i < MAX_CELLS; i++) {
				cells[i] = new Cell();
			}
		}

		void feed(float flow) {
			cells[0]->volume = clamp(cells[0]->volume + flow, 0.f, maxVolume);
		}

		void leak(float viscosity) {
			for (int i = MAX_CELLS - 1; i >= 0; i--) {
				leakCell(i, viscosity);
			}
		}

		void leakCell(int n, float viscosity) {
			float leakage = 0.0;
			if (cells[n]->volume >= viscosity) {
				cells[n]->drop = true;
				leakage = cells[n]->volume;
			} else {
				cells[n]->drop = false;
				float pressure = 0.0;
				for (int i = 0; i < n; i++) {
					pressure += cells[i]->volume;
				}

				float leakage = pressure / viscosity;

				if (leakage > cells[n]->volume) {
					leakage = cells[n]->volume;
				}
			}

			if (n == MAX_CELLS - 1) {
				cells[n]->volume -= leakage;
			} else {
				if (cells[n + 1]->volume + leakage > maxVolume) {
					leakage = maxVolume - cells[n + 1]->volume;
				}
				cells[n + 1]->volume += leakage;
				cells[n]->volume -= leakage;
			}
		}
	};

	Pipe * pipe;
	dsp::SchmittTrigger trigger;

	Droplets() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FLOW_PARAM, 0.f, MAX_FLOW, 0.f, "Flow");
		configParam(FLOW_MOD_PARAM, -1.f, 1.f, 0.f, "Flow modulation amount");
		configParam(VISC_PARAM, 0.001f, MAX_VISC, 0.1f, "Viscosity");
		configParam(VISC_MOD_PARAM, -1.f, 1.f, 0.f, "Viscosity modulation amount");
		configInput(FLOW_INPUT, "Flow modulation");
		configInput(VISC_INPUT, "Viscosity modulation");
		configInput(TICK_INPUT, "Tick");
		configOutput(DROP_OUTPUT, "Drop");
		configOutput(CV_OUTPUT, "Poly CV");
		pipe = new Pipe();
	}

	void process(const ProcessArgs& args) override {
		bool triggered = trigger.process(inputs[TICK_INPUT].getVoltage(), 0.1f, 2.f);
		triggered &= trigger.isHigh();
		if (!triggered) {
			return;
		}

		float flow = params[FLOW_PARAM].getValue();
		if (inputs[FLOW_INPUT].isConnected()) {
			float mod = params[FLOW_MOD_PARAM].getValue();
			flow += inputs[FLOW_INPUT].getVoltage() * mod * MAX_FLOW / 10.0;
		}
		flow = clamp(flow, 0.f, MAX_FLOW);

		float viscosity = params[VISC_PARAM].getValue();
		if (inputs[VISC_INPUT].isConnected()) {
			float mod = params[VISC_MOD_PARAM].getValue();
			viscosity += inputs[VISC_INPUT].getVoltage() * mod * MAX_VISC / 10.0;
		}
		viscosity = clamp(viscosity, 0.f, MAX_VISC);

		pipe->leak(viscosity);
		pipe->feed(flow);

		outputs[DROP_OUTPUT].setChannels(8);
		outputs[CV_OUTPUT].setChannels(8);

		for (int i = 0; i < 8; i++) {
			outputs[CV_OUTPUT].setVoltage(pipe->cells[i]->volume, i);
			outputs[DROP_OUTPUT].setVoltage(pipe->cells[i]->drop ? 10.f : 0.f, i);
			lights[i].setBrightness(pipe->cells[i]->volume / 10.f);
		}
	}

	void onReset(const ResetEvent& e) override {
		delete pipe;
		pipe = new Pipe();
	}
};

struct DropletsWidget : ModuleWidget {
	DropletsWidget(Droplets* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Droplets.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 23.0)), module, Droplets::FLOW_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 24.0)), module, Droplets::FLOW_MOD_PARAM));
		addParam(createParamCentered<FlatKnobStd>(mm2px(Vec(13.0, 43.0)), module, Droplets::VISC_PARAM));
		addParam(createParamCentered<FlatSliderMod>(mm2px(Vec(6.0, 44.0)), module, Droplets::VISC_MOD_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 16.0)), module, Droplets::FLOW_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 36.0)), module, Droplets::VISC_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(6.0, 56.0)), module, Droplets::TICK_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(25.0, 99.0)), module, Droplets::DROP_OUTPUT));
		addOutput(createOutputCentered<PolyOutlet>(mm2px(Vec(25.0, 107.0)), module, Droplets::CV_OUTPUT));

		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 20.0)), module, Droplets::L0_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 26.0)), module, Droplets::L1_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 32.0)), module, Droplets::L2_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 38.0)), module, Droplets::L3_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 44.0)), module, Droplets::L4_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 50.0)), module, Droplets::L5_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 56.0)), module, Droplets::L6_LIGHT));
		addChild(createLightCentered<FlatLightSquareBig<BlackWhiteLight>>(mm2px(Vec(25.0, 62.0)), module, Droplets::L7_LIGHT));
	}
};


Model* modelDroplets = createModel<Droplets, DropletsWidget>("Droplets");