#ifndef LOGIC_H
#define LOGIC_H

#include "raylib.h"


#define CIRCUT_NAME_LEN 20
#define PIN_NAME_LEN 15

#define DEFAULT_CIRCUIT_CAP 20

#define DEFAULT_PIN_CAP 5

#define DEFAULT_COMPONENT_CAP 5

#define DEFAULT_WIRE_CAP 5

#define DEFAULT_PIN_CONNECTED_CAP 2

#define DEFAULT_WIRE_PATH_LEN 5

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
    char name[PIN_NAME_LEN+1];
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

    Vector2 dimentions;
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
    Vector2 *pathStart;
    Vector2 *pathEnd;
    int pathLen;
    Color color;
    int thickness;
} Wire;

typedef struct LogicCtx {
    struct Circuit **circuits;
    int nCircuits;
    int capCircuits;
    struct Component *evalQueue;

    bool componentDragging;
    Circuit *draggedComponent;
    Vector2 componentPos;
    Color componentColor;

    bool buildingWire;
    bool pathDir;
    Vector2 wirePath[PATH_SIZE];
    int pathLen;
    Color wireColor;
    int wireThickness;
    Pin *wireSrcPin;
    Pin *wireDestPin;

    bool hovering;

    Circuit *viewedCircuit;
} LogicCtx;

LogicCtx *logicInit();
void logicFree(LogicCtx *ctx);
Circuit *addCircuit(LogicCtx *ctx, Circuit *circuit, void (*eval)(Circuit *circuit));
Pin *newPin(LogicCtx *ctx, Circuit *circuit, char *name, PinType type);
Pin *getPinCircuit(Circuit *circuit, char *name, PinType type);
Pin *getPinComponent(Component *component, char *name, PinType type);
int connectPins(LogicCtx *ctx, Pin *pin1, Pin *pin2);
Component *addComponent(LogicCtx *ctx, Circuit *dest, Circuit *src);

Circuit *getCircuit(LogicCtx *ctx, char *name);

Circuit *createCircuit(char *name, void (*eval)(Circuit *circuit));
Component *createComponent(Circuit *defn);
Pin *createPin(char *name, Component *component, PinType type);

void deleteCircuit(Circuit *circuit);
void deleteComponent(Component *component);
void deletePin(Pin *pin);

void insertCircuit(LogicCtx *ctx, Circuit *circuit);
void insertConnected(Pin *pin1, Pin *pin2);
void insertWire(LogicCtx *ctx);

bool isConnected(Pin *pin1, Pin *pin2);

#endif
