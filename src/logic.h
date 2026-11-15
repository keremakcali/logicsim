#ifndef LOGIC_H
#define LOGIC_H

#include "raylib.h"
#include <stdbool.h>
#include <raylib.h>

#define EVAL_BUFF_CAP 30

#define CIRCUT_NAME_LEN 20

#define DEFAULT_CIRCUIT_CAP 20

#define DEFAULT_PIN_CAP 5

#define DEFAULT_COMPONENT_CAP 5

#define DEFAULT_WIRE_CAP 5

#define DEFAULT_PIN_CONNECTED_CAP 2

#define DEFAULT_WIRE_PATH_LEN 10

#define PIN_PAD 8
#define PIN_RAD 6

#define CIRCUIT_WIDTH 60

#define PATH_SIZE 10

#define MAX_COMPONENT_EVAL_COUNT 5

typedef enum PinState {
    LOW,
    HIGH,
    FLOATING,
} PinState;

typedef enum PinType {
    IN,
    OUT,
} PinType;

typedef struct Pin {
    PinType type;
    bool io;
    enum PinState state;
    struct Component *component;

    struct Pin **connected;
    int nConnected;
    int capConnected;

    Vector2 pos;

} Pin;

typedef struct Circuit {
    char name[CIRCUT_NAME_LEN+1];

    struct Pin **pins;
    int nPins;
    int capPins;

    int nInPins;
    int nOutPins;

    struct Component **components;
    int nComponents;
    int capComponents;

    struct Wire *wires;
    int nWires;
    int capWires;

    void (*eval)(struct Circuit *circuit);


} Circuit;

typedef struct Component {
    struct Circuit *defn;
    int id;

    Vector2 pos;
    Color color;

    int nInPins;
    int nOutPins;
    int nPins;
    struct Pin *pins[];

} Component;


typedef struct Wire {
    Circuit *circuit;
    Pin *startPin;
    Pin *endPin;
    Color color;
    int pathLen;
    Vector2 path[PATH_SIZE];
} Wire;

typedef struct LogicCtx {
    struct Circuit **circuits;
    int nCircuits;
    int capCircuits;
} LogicCtx;

void logicInit(LogicCtx *ctx);
void logicFree(LogicCtx *ctx);
Circuit *addCircuit(LogicCtx *ctx, Circuit *circuit);
Pin *newPin(Circuit *circuit, PinType type);
int connectPins(Circuit *circuit, Pin *pin1, Pin *pin2, Wire *wire);
Component *addComponent(LogicCtx *ctx, Circuit *dest, Component *component);

Circuit *getCircuit(LogicCtx *ctx, char *name);

Circuit *createCircuit(char *name, void (*eval)(Circuit *circuit));
Component *createComponent(Circuit *defn);
Pin *createPin(Component *component, PinType type);

void deleteCircuit(Circuit *circuit);
void deleteComponent(Component *component);
void deletePin(Pin *pin);

void insertCircuit(LogicCtx *ctx, Circuit *circuit);
void insertConnected(Pin *pin1, Pin *pin2);
void insertWire(Circuit *circuit, Wire *wire);

bool isConnected(Pin *pin1, Pin *pin2);

void logicEval(Circuit *circuit);
void handlePinChange(LogicCtx *ctx, Pin *pin);
PinState calculatePinState(Pin *pin);
void printEvalQueue(Circuit *circuit);
bool isComponentInBuff(Component **buff,  Component *component, int size);
void setPinState(Pin *pin, PinState state);

#endif
