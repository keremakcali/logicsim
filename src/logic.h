#ifndef LOGIC_H
#define LOGIC_H

#include "raylib.h"

#define EVAL_QUEUE_CAP 200

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

typedef enum PinState {
    LOW,
    HIGH,
} PinState;

typedef enum PinType {
    IN,
    OUT,
} PinType;

typedef struct Pin {
    PinType type;
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

    struct Pin **pins;
    int nPins;
    int nInPins;
    int nOutPins;

    struct Component *next;

    Vector2 pos;
    Color color;
} Component;


typedef struct Wire {
    Vector2 path[DEFAULT_WIRE_PATH_LEN];
    Pin *startPin;
    Pin *endPin;
    int pathLen;
    Color color;
} Wire;

typedef struct LogicCtx {
    struct Circuit **circuits;
    int nCircuits;
    int capCircuits;
    struct Component **evalQueue;
    int queueSize;
    int queueCap;
} LogicCtx;

LogicCtx *logicInit();
void logicFree(LogicCtx *ctx);
Circuit *addCircuit(LogicCtx *ctx, Circuit *circuit);
Pin *newPin(Circuit *circuit, PinType type);
int connectPins(Circuit *circuit, Pin *pin1, Pin *pin2, Wire *wire);
Component *addComponent(Circuit *dest, Component *component);

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

void handlePinChange(LogicCtx *ctx, Pin *pin);
int calculatePinState(Pin *pin);
void evalComponent(Component *component);
void logicEval(LogicCtx *ctx);

#endif
