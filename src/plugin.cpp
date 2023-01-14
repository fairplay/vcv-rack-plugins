#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelMicroLooper);
	p->addModel(modelLogisticScratch);
	p->addModel(modelLFSR8);
	p->addModel(modelLFSR16);
	p->addModel(modelDroplets);
	p->addModel(modelChaos);
	
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
