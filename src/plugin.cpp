#include "plugin.hpp"

Plugin* pluginInstance;

void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelLogisticScratch);
	p->addModel(modelLFSR8Poly);
	p->addModel(modelLFSR16Poly);
	p->addModel(modelDroplets);
	p->addModel(modelChaosMaps);
	p->addModel(modelMuLooper);
	p->addModel(modelPluck);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
