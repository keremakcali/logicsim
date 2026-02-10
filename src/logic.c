#include "logic.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>


LogicCtx *logicInit() {
    LogicCtx *ctx = malloc(sizeof(LogicCtx));
    if (!ctx) {
        return NULL;
    }
    Circuit **circuits = malloc( DEFAULT_CIRCUIT_CAP * sizeof(Circuit *));
    if (!circuits) {
        free(ctx);
        return NULL;
    }
    ctx->circuits = circuits;
    ctx->nCircuits = 0;
    ctx->capCircuits = DEFAULT_CIRCUIT_CAP;


    Component **evalQueue = malloc( EVAL_QUEUE_CAP * sizeof(Component * ));
    if (!evalQueue) {
        free(ctx);
        free(circuits);
        return NULL;
    }
    ctx->evalQueue = evalQueue;
    ctx->queueSize = 0;
    ctx->queueCap = EVAL_QUEUE_CAP;


    return ctx;

}

void logicFree(LogicCtx *ctx) {
    assert(ctx);

    Circuit **circuits = ctx->circuits;
    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        deleteCircuit(circuits[i]);
    }

    free(circuits);
    free(ctx);
}

Circuit *addCircuit(LogicCtx *ctx, Circuit *circuit) {
    assert(ctx);
    assert(circuit);


    if (getCircuit(ctx, circuit->name)) {
        deleteCircuit(circuit);
        return NULL;
    }


    insertCircuit(ctx, circuit); 

    return circuit;
}

void insertCircuit(LogicCtx *ctx, Circuit *circuit) {
    if (ctx->nCircuits >= ctx->capCircuits) {
        Circuit **circuits = realloc(ctx->circuits, 2 * sizeof(Circuit *) * ctx->capCircuits);
        // if (!circuits) return NULL;
        ctx->circuits = circuits;
        ctx->capCircuits *= 2;
    }

    ctx->circuits[ctx->nCircuits++] = circuit;
}

Component *addComponent(Circuit *dest, Component *component) {
    assert(dest);
    assert(component);

    
    if (dest->nComponents >= dest->capComponents) {
        Component **components = realloc(dest->components, 2 * sizeof(Component *) * dest->capComponents); 
        if (!components) {
            deleteComponent(component);
            return NULL;
            }
        dest->components = components;
        dest->capComponents *= 2;
    }

    dest->components[dest->nComponents++] = component;

    return component;
}

Pin *newPin(Circuit *circuit, PinType type) {
    assert(circuit);

    Pin *pin = createPin(NULL, type);
    if (!pin) {
        return NULL;
    }

    if (circuit->nPins >= circuit->capPins) {
        Pin **pins = realloc(circuit->pins, 2 * sizeof(Pin *) * circuit->capPins);
        if (!pins) return NULL;
        circuit->pins = pins;
        circuit->capPins *= 2;
    }

    circuit->pins[circuit->nPins++] = pin;
    if (type == IN) circuit->nInPins++;
    if (type == OUT) circuit->nOutPins++;

    return pin;
}

Circuit *getCircuit(LogicCtx *ctx, char *name) {
    assert(ctx);
    assert(name);


    Circuit **circuits = ctx->circuits;
    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        Circuit *circuit = circuits[i];
        if (!strcmp(circuit->name, name)) 
            return circuit;
    }
    return NULL;
}


int connectPins(Circuit *circuit, Pin *pin1, Pin *pin2, Wire *wire) {
    assert(pin1);
    assert(pin2);

    if (pin1->type == pin2->type) return 0;

    if (isConnected(pin1, pin2)) return 0;


    insertConnected(pin1, pin2);
    insertConnected(pin2, pin1);
    insertWire(circuit, wire);
    return 1;
}

bool isConnected(Pin *pin1, Pin *pin2) {
    assert(pin1);
    assert(pin2);

    Pin **connected = pin1->connected;
    for (int i = pin1->nConnected-1; i >= 0 ; i--) {
        if (connected[i] == pin2) {
            return true;
        } 
    }
    return false;
}


void insertConnected(Pin *pinMain, Pin *pinConnected) {
    assert(pinMain);
    assert(pinConnected);

    if (pinMain->nConnected == pinMain->capConnected) {
        Pin **connected = realloc(pinMain->connected, 2 * sizeof(Pin *) * pinMain->capConnected);
        if (!connected) return;
        pinMain->connected = connected;
        pinMain->capConnected *= 2;
    }
    pinMain->connected[pinMain->nConnected++] = pinConnected;
}

void insertWire(Circuit *circuit, Wire *wire) {
    assert(circuit);

    if (circuit->nWires == circuit->capWires) {
        Wire *wires = realloc(circuit->wires, sizeof(Wire) * 2 * circuit->nWires);
        if (!wires) {
            return;
        }
        circuit->wires = wires;
        circuit->capWires *= 2;
    }
    circuit->wires[circuit->nWires++] = *wire;
}

Circuit *createCircuit(char *name, void (*eval)(Circuit *circuit)) {
    assert(name);

    Circuit *newCirc = malloc(sizeof(Circuit));
    if (!newCirc) {
        return NULL;
    }

    strncpy(newCirc->name, name, CIRCUT_NAME_LEN);
    Pin **pins = malloc(DEFAULT_PIN_CAP * sizeof(Pin *));
    if (!pins) {
        free(newCirc);
        return NULL;
    }

    newCirc->pins = pins;

    newCirc->nPins = 0;
    newCirc->capPins = DEFAULT_PIN_CAP;
    newCirc->nInPins = 0;
    newCirc->nOutPins = 0;

    Component **components = malloc(DEFAULT_COMPONENT_CAP * sizeof(Component *));
    if (!components) {
        free(newCirc);
        free(pins);
        return NULL;
    }
    newCirc->components = components;
    newCirc->capComponents = DEFAULT_COMPONENT_CAP;
    newCirc->nComponents = 0;

    Wire *wires = malloc(DEFAULT_WIRE_CAP * sizeof(Wire));
    if (!wires) {
        free(newCirc);
        free(pins);
        free(components);
        return NULL;
    }
    newCirc->wires = wires;
    newCirc->nWires = 0;
    newCirc->capWires = DEFAULT_WIRE_CAP;

    newCirc->eval = eval;


    return newCirc;

}

Component *createComponent(Circuit *defn) {
    assert(defn);

    Component *component = malloc(sizeof(Component));
    if (!component) {
        return NULL;
    }
    int nPins = defn->nPins;

    Pin **pins = malloc(nPins * sizeof(Pin *));
    if (!pins) {
        free(component);
        return NULL;
    }

    component->defn = defn;
    component->next = NULL;

    component->pins = pins;
    component->nPins = defn->nPins;
    component->nInPins = defn->nInPins;
    component->nOutPins = defn->nOutPins;
    for (int i = nPins-1; i >= 0; i--) {
        Pin *srcPin = defn->pins[i];
        Pin *newPin = createPin(component, srcPin->type);
        component->pins[i] = newPin;
    }

    return component;
}

Pin *createPin(Component *component, PinType type) {

    Pin *newPin = malloc(sizeof(Pin));
    if (!newPin) {
        return NULL;
    }

    newPin->connected = malloc(DEFAULT_PIN_CONNECTED_CAP * sizeof(Pin *));
    newPin->component = component;
    newPin->nConnected = 0;
    newPin->capConnected = DEFAULT_PIN_CONNECTED_CAP;
    newPin->state = LOW;
    newPin->type = type;

    return newPin;

}

void handlePinChange(LogicCtx *ctx, Pin *pin) {
    assert(ctx);
    assert(pin);

    for (int i = 0; i < pin->nConnected; i++) {
        Pin *connectedPin = pin->connected[i];
        calculatePinState(connectedPin);
        if (connectedPin->component) {
            if (ctx->queueSize < ctx->queueCap) {
                ctx->evalQueue[ctx->queueSize++] = connectedPin->component;
            }   
        } 
    }
}

int calculatePinState(Pin *pin) {
    if (pin->type != IN) return -1;
    for (int i = 0; i < pin->nConnected; i++) {
        Pin *connectedPin = pin->connected[i];
        if (connectedPin->state == 1) {
            pin->state = 1;
            return 1;
        }
    }
    pin->state = 0;
    return 0;
}

void evalComponent(Component *component) {
    for (int i = 0; i < component->nPins; i++) {
        Pin *pin = component->pins[i];
        calculatePinState(pin);
        component->defn->pins[i]->state = pin->state;
    }
    if (component->defn->eval) {
        component->defn->eval(component->defn);
        for (int i = 0; i < component->nPins; i++) {
            if (component->pins[i]->type == OUT) {
                component->pins[i] = component->defn->pins[i];
            }
        }
    }
}

void logicEval(LogicCtx *ctx) {
    for (int i = 0; i < ctx->queueSize; i++) {
        evalComponent(ctx->evalQueue[i]);
    }
    ctx->queueSize = 0;
}


void deleteCircuit(Circuit *circuit) {
    assert(circuit);

    Pin **pins = circuit->pins;
    for (int i = circuit->nPins-1; i >= 0; i--) {
        deletePin(pins[i]);
    }
    free(pins);

    Component **components = circuit->components;
    for (int i = circuit->nComponents-1; i >= 0; i--) {
        deleteComponent(components[i]);
    }
    free(components);

    free(circuit->wires);

    free(circuit);
}

void deleteComponent(Component *component) {
    assert(component);

    Pin **pins = component->pins;
    for (int i = component->nPins-1; i >= 0; i--) {
        deletePin(pins[i]);
    }
    free(pins);

    free(component);
}

void deletePin(Pin *pin) {
    assert(pin);

    free(pin->connected);
    free(pin);
}


