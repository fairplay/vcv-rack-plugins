#include "plugin.hpp"

struct Droplets : Module {
	enum ParamId {
		FLOW_PARAM,
		VISCOSITY_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		GATE_INPUT,
		FLOW_INPUT,
		VISCOSITY_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		DROP_OUTPUT,
		CV_VELOCITY_OUTPUT,
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

	Droplets() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FLOW_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VISCOSITY_PARAM, 0.1f, 1.f, 0.1f, "");
		configInput(GATE_INPUT, "");
		configInput(FLOW_INPUT, "");
		configInput(VISCOSITY_INPUT, "");
		configOutput(DROP_OUTPUT, "");
		configOutput(CV_VELOCITY_OUTPUT, "");
	}

	bool triggered = false;
	dsp::SchmittTrigger trigger;
	float state[8] = {0.f};
	float viscosity;
	float flow;
	float drop = false;

	float getVelocity(int n) {
		if (state[n] > viscosity) {
			drop = true;
			return state[n];
		} else {
			drop = false;
		}


		float pressure = flow;
		for (int i = n + 1; i < 8; i++) {
			pressure += state[i];
		}

		float velocity = pressure / viscosity / 10.f;

		if (velocity > state[n]) {
			velocity = state[n];
		}

		if (n > 0 && state[n - 1] + velocity > 1.f) {
			velocity = 1.f - state[n - 1];
		}

		return velocity;
	}

	void setLeds() {
		for (int i = L0_LIGHT; i <= L7_LIGHT; i++) {
			lights[i].setBrightness(state[i]);
		}
	}

	void process(const ProcessArgs& args) override {
		if (trigger.process(inputs[GATE_INPUT].getVoltage(), 0.1f, 2.f)) {
			triggered ^= true;
		}
		if (!triggered) {
			return;
		}

		viscosity = params[VISCOSITY_PARAM].getValue();
		if (inputs[VISCOSITY_INPUT].isConnected()) {
			viscosity = inputs[VISCOSITY_INPUT].getVoltage() / 10.f;
			viscosity = clamp(viscosity, 0.f, 1.f);
		}

		flow = params[FLOW_PARAM].getValue();
		if (inputs[FLOW_INPUT].isConnected()) {
			flow += inputs[FLOW_INPUT].getVoltage() / 10.f;
			flow = clamp(flow, 0.f, 1.f);
		}

		//float leakVelocity = 0.f;

		for (int i = 0; i < 8; i++) {
			float velocity = getVelocity(i);
			state[i] -= velocity;
			if (i > 0) {
				state[i - 1] += velocity;
			} else {
				//leakVelocity = velocity;
			}

		}
		state[7] = clamp(state[7] + flow, 0.f, 1.f);

		float out = 0.f;
		for (int i = 0; i < 8; i++) {
			out = 2.f * out + state[i];
		}

		outputs[DROP_OUTPUT].setVoltage(drop ? 10.f : 0.f);
		outputs[CV_VELOCITY_OUTPUT].setVoltage(out / 256.f);
		//outputs[CV_VELOCITY_OUTPUT].setVoltage(leakVelocity);

		setLeds();
		triggered = false;
	}

	void onReset(const ResetEvent& e) override {
		std::fill(state, state + 8, 0.f);
		setLeds();
	}
};

struct DropletsWidget : ModuleWidget {
	DropletsWidget(Droplets* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Droplets.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<TsKnobLarge>(mm2px(Vec(28.0, 42.0)), module, Droplets::FLOW_PARAM));
		addParam(createParamCentered<TsKnobLarge>(mm2px(Vec(28.0, 72.0)), module, Droplets::VISCOSITY_PARAM));

		addInput(createInputCentered<Inlet>(mm2px(Vec(7.5, 15.0)), module, Droplets::GATE_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(21.0, 35.0)), module, Droplets::FLOW_INPUT));
		addInput(createInputCentered<Inlet>(mm2px(Vec(21.0, 65.0)), module, Droplets::VISCOSITY_INPUT));

		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.08, 90.0)), module, Droplets::DROP_OUTPUT));
		addOutput(createOutputCentered<Outlet>(mm2px(Vec(35.08, 100.0)), module, Droplets::CV_VELOCITY_OUTPUT));

		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 35.0)), module, Droplets::L6_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 45.0)), module, Droplets::L5_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 55.0)), module, Droplets::L4_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 65.0)), module, Droplets::L3_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 75.0)), module, Droplets::L2_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 85.0)), module, Droplets::L1_LIGHT));
		addChild(createLightCentered<TsLightSquareLarge<BlueLight>>(mm2px(Vec(7.5, 95.0)), module, Droplets::L0_LIGHT));
	}
};


Model* modelDroplets = createModel<Droplets, DropletsWidget>("Droplets");