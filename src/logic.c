#include "logic.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "config.h"


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

    ctx->evalQueue = NULL;

    ctx->viewedCircuit = NULL;
    
    ctx->pathLen = 0;

    ctx->pathDir = true;

    ctx->componentDragging = false;
    ctx->buildingWire = false;

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

Circuit *addCircuit(LogicCtx *ctx, Circuit *circuit, void (*eval)(Circuit *circuit)) {
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

Component *addComponent(LogicCtx *ctx, Circuit *dest, Circuit *component) {
    assert(ctx);
    assert(dest);
    assert(component);

    Component *newCmp = createComponent(component);
    if (!newCmp) {
        return NULL;
    }
    
    if (dest->nComponents >= dest->capComponents) {
        Component **components = realloc(dest->components, 2 * sizeof(Component *) * dest->capComponents); 
        if (!components) {
            deleteComponent(newCmp);
            return NULL;
            }
        dest->components = components;
        dest->capComponents *= 2;
    }

    dest->components[dest->nComponents++] = newCmp;

    return newCmp;
}

Pin *newPin(LogicCtx *ctx, Circuit *circuit, char *name, PinType type) {
    assert(ctx);
    assert(circuit);
    assert(name);

    if (getPinCircuit(circuit, name, type)) {
        return NULL;
    }

    Pin *pin = createPin(name, NULL, type);
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

    if (type == IN && circuit->nInPins > circuit->nOutPins) {
        circuit->dimentions.y += 2*ComponentPinRad + ComponentPinPad;
    } else if (type == OUT && circuit->nOutPins > circuit->nInPins) {
        circuit->dimentions.y += 2*ComponentPinRad + ComponentPinRad;
    }

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

Pin *getPinCircuit(Circuit *circuit, char *name, PinType type) {
    assert(circuit);
    assert(name);

    for (int i = circuit->nPins-1; i >= 0; i--) {
        Pin *pin = circuit->pins[i];
        if (pin->type == type && !strcmp(pin->name, name))
            return pin;
    }
    return NULL;
}



Pin *getPinComponent(Component *component, char *name, PinType type) {
    assert(component);
    assert(name);

    for (int i = component->nPins-1; i >= 0; i--) {
        Pin *pin = component->pins[i];
        if (pin->type == type && !strcmp(pin->name, name))
            return pin;
    }
    return NULL;
}

int connectPins(LogicCtx *ctx, Pin *pin1, Pin *pin2) {
    assert(pin1);
    assert(pin2);

    if (pin1->type == pin2->type && pin1->component && pin2->component) return 0;
    if (pin1->type != pin2->type && (pin1->component && !pin2->component || !pin1->component && pin2->component)) return 0;

    if (isConnected(pin1, pin2)) return 0;


    insertConnected(pin1, pin2);
    insertConnected(pin2, pin1);
    insertWire(ctx);
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

void insertWire(LogicCtx *ctx) {
    assert(ctx);

    Circuit *circ = ctx->viewedCircuit;
    if (!circ) {
        return;
    }
    if (circ->nWires == circ->capWires) {
        Wire *wires = realloc(circ->wires, sizeof(Wire) * 2 * circ->nWires);
        if (!wires) {
            return;
        }
        circ->wires = wires;
        circ->capWires *= 2;
    }
    Wire *wire = &circ->wires[circ->nWires++];
    for (int i = 0; i < ctx->pathLen; i++) {
        wire->path[i] = ctx->wirePath[i];
    }
    wire->pathStart = &(ctx->wireSrcPin->pos);
    wire->pathEnd = &(ctx->wireDestPin->pos);
    wire->pathLen = ctx->pathLen;
    wire->thickness = ctx->wireThickness;
    wire->color = ctx->wireColor;

    ctx->buildingWire = false;
    ctx->pathLen = 0;
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

    newCirc->dimentions.x = CIRCUIT_WIDTH;
    newCirc->dimentions.y = PIN_PAD;

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
        Pin *newPin = createPin(srcPin->name, srcPin->component, srcPin->type);
        component->pins[i] = newPin;
    }

    return component;
}

Pin *createPin(char *name, Component *component, PinType type) {
    assert(name);

    Pin *newPin = malloc(sizeof(Pin));
    if (!newPin) {
        return NULL;
    }

    strncpy(newPin->name, name, PIN_NAME_LEN);

    newPin->connected = malloc(DEFAULT_PIN_CONNECTED_CAP * sizeof(Pin *));
    newPin->component = component;
    newPin->nConnected = 0;
    newPin->capConnected = DEFAULT_PIN_CONNECTED_CAP;
    newPin->state = LOW;
    newPin->type = type;

    return newPin;

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


