#include "logic.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>


void logicInit(LogicCtx *ctx) {
    assert(ctx);

    Circuit **circuits = malloc( DEFAULT_CIRCUIT_CAP * sizeof(Circuit *));
    if (!circuits) {
        return;
    }
    ctx->circuits = circuits;
    ctx->nCircuits = 0;
    ctx->capCircuits = DEFAULT_CIRCUIT_CAP;
    
}

void logicFree(LogicCtx *ctx) {
    assert(ctx);

    Circuit **circuits = ctx->circuits;
    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        deleteCircuit(circuits[i]);
    }

    free(circuits);
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

Component *addComponent(LogicCtx *ctx, Circuit *dest, Component *component) {
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

    for (int i = 0; i < component->nPins; i++) {
            if (dest->nPins >= dest->capPins) {
                dest->pins = realloc(dest->pins, dest->capPins * 2 * sizeof(Pin *));
                dest->capPins *= 2;
            }
            dest->pins[dest->nPins++] = component->pins[i];
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

    int a = pin1->component ? 1: 0;
    int b = pin2->component ? 1: 0;
    switch (a+b){
        case 1:
            if (pin1->type != pin2->type) return 0;
            break;
        case 2:
            if (pin1->type == pin2->type) return 0;
            break;
    }

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

    wire->circuit = circuit;
    if (circuit->nWires == circuit->capWires) {
        Wire *wires = realloc(circuit->wires, sizeof(Wire) * 2 * circuit->nWires);
        if (!wires) {
            return;
        }
        circuit->wires = wires;
        circuit->capWires *= 2;
    }
    if (!wire->startPin || !wire->endPin) {
        return;
    }
    
    circuit->wires[circuit->nWires++] = *wire;
}

Circuit *createCircuit(char *name, void (*eval)(Circuit *circuit)) {
    assert(name);

    Circuit *newCirc = malloc(sizeof(Circuit));
    if (!newCirc) {
        return NULL;
    }

    newCirc->name[CIRCUIT_NAME_LEN] = '\0';
    strncpy(newCirc->name, name, CIRCUIT_NAME_LEN);
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

    Component *component = malloc(sizeof(Component) + defn->nPins * sizeof(struct Pin *));
    if (!component) {
        return NULL;
    }
    component->defn = defn;

    component->nPins = defn->nPins;
    component->nInPins = defn->nInPins;
    component->nOutPins = defn->nOutPins;

    for (int i = 0; i < component->defn->nPins; i++) {
        Pin *srcPin = component->defn->pins[i];
        Pin *newPin = createPin(component, srcPin->type);
        if (!srcPin->component) {
            newPin->io = true;
        } else {
            newPin->io = false;
        }
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
    newPin->io = false;

    return newPin;

}

PinState calculatePinState(Pin *pin) {
if (!pin->component && pin->type == OUT || pin->component && pin->type == IN) {
        for (int i = 0; i < pin->nConnected; i++) {
            Pin *connectedPin = pin->connected[i];
            if (connectedPin->state == HIGH) {
                return HIGH;
            }
        }
    }
    return LOW;
}

void logicEval(Circuit *circuit) {
    for (int i = 0; i < circuit->nPins; i++) {
        Pin *pin = circuit->pins[i];
        if (pin->io && pin->component && pin->type == IN) {
            setPinState(pin, calculatePinState(pin));
        }
    }

    for (int i = 0; i < circuit->nComponents; i++) {
        Component *component = circuit->components[i];
        for (int j = 0; j < component->nPins; j++) {
            setPinState(component->defn->pins[j], component->pins[j]->state);
        }
        if (component->defn->eval) {
            component->defn->eval(component->defn);
        } else {
            logicEval(component->defn);
        }
        for (int j = 0; j < component->nPins; j++) {
            setPinState(component->pins[j], component->defn->pins[j]->state);
        }
    }

    for (int i = 0; i < circuit->nPins; i++) {
        Pin *pin = circuit->pins[i];
        if ((!pin->component && pin->type == OUT) || (pin->io && pin->component && pin->type == IN)) {
            setPinState(pin, calculatePinState(pin));
        }
    }
}

    

void setPinState(Pin *pin, PinState state) {     
    pin->state = state;
}


bool isComponentInBuff(Component **buff,  Component *component, int size) {
    for (int i = 0; i < size; i++) {
        Component *c = buff[i];
        if (component == c) {
            return true;
        }
    }
    return false;
}

// void printEvalQueue(Circuit *circuit) {
//     for (int i = 0; i <  circuit->evalBuffSize; i++) {
//         Component *component = circuit->evalBuff[i];
//         if (component) {
//         } else {
//         }
//     }
// }


void deleteCircuit(Circuit *circuit) {
    assert(circuit);

    Pin **pins = circuit->pins;
    for (int i = circuit->nPins-1; i >= 0; i--) {
        Pin *pin = pins[i];
        if (pin->component ) continue;
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

    free(component);
}

void deletePin(Pin *pin) {
    assert(pin);

    free(pin->connected);
    free(pin);
}



