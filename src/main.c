#include <math.h>
#include <stddef.h>

#include <raylib.h>
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

#include "logic.h"


void DrawControls(LogicCtx *ctx);
void DrawEditor(LogicCtx *ctx);

void DrawCircuit(Circuit *circuit, Vector2 pos);
void DrawComponent(LogicCtx *ctx, Component *component, bool drawPins);
void DrawPin(LogicCtx *ctx, Pin *pin, bool manualState);
void DrawWire(Wire *wire);

void HandleComponentDrag(LogicCtx *ctx);
void HandleWireBuild();

const int ScreenWidth = 1270;
const int ScreenHeight = 720;

const float SideBarWidth = 250;
const float SideBarButtonHeight = 50;
const float SideBarComponentPadding = 30;

const float ComponentPinRad = 10;
const float ComponentPinBorder = 2;
const float ComponentPinPad = 5;

bool circuitNameInput = false;

bool hovering = false;
bool mouseConsumed = false;

Circuit *viewedCircuit = NULL;
Circuit *evalCircuit = NULL;
Component *viewedComponent = NULL;

bool isEditingRoot(void) {
    return viewedCircuit == evalCircuit;
}

bool componentDragging = false;
Component *draggedComponent = NULL;

bool buildingWire = false;
Wire drawnWire;

Color WireColorOn = {255,51,0, 255};
Color WireColorOff = {128,0,0, 255};
Color PinBorderColor = BLACK;

const float SideBarCircuitHeight = 50;

void nandLogic(Circuit *circuit) {
    PinState oldState = circuit->pins[2]->state;
    circuit->pins[2]->state = !(circuit->pins[0]->state && circuit->pins[1]->state);
}

// void norLogic(Circuit *circuit) {
//     circuit->pins[2]->state = !(circuit->pins[0]->state || circuit->pins[1]->state);
// }
//
// void notLogic(Circuit *circuit) {
//     circuit->pins[1]->state = !circuit->pins[0]->state;
// }
//
// void orLogic(Circuit *circuit) {
//     circuit->pins[2]->state = circuit->pins[0]->state || circuit->pins[1]->state;
// }
//
// void andLogic(Circuit *circuit) {
//     PinState oldState = circuit->pins[2]->state;
//     circuit->pins[2]->state = circuit->pins[0]->state && circuit->pins[1]->state;
// }


int main() {

    LogicCtx ctx;
    logicInit(&ctx);

    Circuit *nand = createCircuit("NAND", nandLogic);
    newPin(nand, IN);
    newPin(nand, IN);
    newPin(nand, OUT);
    addCircuit(&ctx, nand);

    // Circuit *and = createCircuit("AND", andLogic);
    // newPin(and, IN);
    // newPin(and, IN);
    // newPin(and, OUT);
    // addCircuit(&ctx, and);
    //
    //
    // Circuit *nor = createCircuit("NOR", norLogic);
    // newPin(nor, IN);
    // newPin(nor, IN);
    // newPin(nor, OUT);
    // addCircuit(&ctx, nor);
    //
    // Circuit *or = createCircuit("or", orLogic);
    // newPin(or, IN);
    // newPin(or, IN);
    // newPin(or, OUT);
    // addCircuit(&ctx, or);
    //
    // Circuit *not = createCircuit("not", notLogic);
    // newPin(not, IN);
    // newPin(not, OUT);
    // addCircuit(&ctx, not);

    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Logic Sim");
    // SetTargetFPS(60);
    SetExitKey(KEY_NULL);


    viewedCircuit = createCircuit("NEW_CIRCUIT", NULL);
    evalCircuit = viewedCircuit;
    drawnWire.color = RED;

    while (!WindowShouldClose()) {
        mouseConsumed = false;

        if (hovering) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            hovering = false;
        } else {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }

        BeginDrawing();
        ClearBackground(GRAY);

        
        HandleWireBuild();

        DrawEditor(&ctx);

        DrawControls(&ctx);

        HandleComponentDrag(&ctx);


        EndDrawing();
        logicEval(evalCircuit);
        if (viewedComponent) {
            for (int i = 0; i < viewedComponent->nPins; i++) {
                setPinState(viewedComponent->defn->pins[i], viewedComponent->pins[i]->state);
            }
        }
    }

    CloseWindow();

    if (draggedComponent) {
        deleteComponent(draggedComponent);
    }
    if (evalCircuit) {
        deleteCircuit(evalCircuit);
    }
    logicFree(&ctx);
}



void DrawControls(LogicCtx *ctx) {
    bool input = false;
    Rectangle panelBounds = {0, 0, SideBarWidth, GetScreenHeight()-2*SideBarButtonHeight};

    Rectangle newButtBounds = {0,GetScreenHeight()-SideBarButtonHeight, SideBarWidth/2, SideBarButtonHeight}; 
    Rectangle saveButtBounds = {SideBarWidth/2,GetScreenHeight()-SideBarButtonHeight, SideBarWidth/2, SideBarButtonHeight}; 
    Rectangle newInPinBounds = {0,GetScreenHeight()-2*SideBarButtonHeight, SideBarWidth/2, SideBarButtonHeight};
    Rectangle newOutPinBounds = {SideBarWidth/2,GetScreenHeight()-2*SideBarButtonHeight, SideBarWidth/2, SideBarButtonHeight};

    // Rectangle inputBounds = {(GetScreenWidth()-SideBarWidth)/2, (GetScreenHeight()-SideBarButtonHeight)/2,SideBarWidth,SideBarButtonHeight}; 
    static char inputStr[29];

    Rectangle content = {0, 0, SideBarWidth - SCROLLBAR_WIDTH, SideBarComponentPadding} ;
    static Vector2 scroll = {0, 0};
    Rectangle view = {0};


    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        Circuit *circuit = ctx->circuits[i];
        content.height += SideBarCircuitHeight + SideBarComponentPadding;
    }

    GuiScrollPanel(panelBounds, "Components", content, &scroll, &view);

    Vector2 cur = {(content.width)/2, 30};
    BeginScissorMode(view.x, view.y, view.width, view.height);
    cur.y += scroll.y;
    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        Circuit *circuit = ctx->circuits[i];
        DrawCircuit(circuit, cur);
        
        cur.y += SideBarCircuitHeight + SideBarComponentPadding;
    }
    EndScissorMode();


    if (circuitNameInput) {
        Rectangle rect = {(GetScreenWidth()-SideBarWidth)/2, (GetScreenHeight()-SideBarButtonHeight)/2, SideBarWidth, SideBarButtonHeight};
        GuiTextBox(rect, evalCircuit->name, CIRCUIT_NAME_LEN, true);
        if (IsKeyPressed(KEY_ENTER)) {
            if (!getCircuit(ctx, evalCircuit->name)) {
                circuitNameInput = false;
                addCircuit(ctx, evalCircuit);
                evalCircuit = createCircuit("NEW_CIRCUIT", NULL);
                viewedCircuit = evalCircuit;
                viewedComponent = NULL;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            circuitNameInput = false;
        }
    }

    if (!isEditingRoot() && IsKeyPressed(KEY_ESCAPE)) {
        viewedCircuit = evalCircuit;
        viewedComponent = NULL;
        buildingWire = false;
    }

    if (GuiButton(newButtBounds, "New Circuit")) {
        if (evalCircuit) {
            deleteCircuit(evalCircuit);
        }
        buildingWire = false;
        circuitNameInput = false;
        evalCircuit = createCircuit("NEW_CIRCUIT", NULL);
        viewedCircuit = evalCircuit;
        viewedComponent = NULL;
    }
    if (isEditingRoot()) {
        if (GuiButton(saveButtBounds, "Save Circuit")) {
            circuitNameInput = true;
            buildingWire = false;
        }
        if (GuiButton(newInPinBounds, "New In Pin")) {
            newPin(evalCircuit, IN);
            inputStr[0] = 0;
        }
        if (GuiButton(newOutPinBounds, "New Out Pin")) {
            newPin(evalCircuit, OUT);
            inputStr[0] = 0;
        }
    } else {
        if (GuiButton(saveButtBounds, "Back")) {
            viewedCircuit = evalCircuit;
            viewedComponent = NULL;
            buildingWire = false;
        }
    }

    // if (nameInput) {
    float width = 100;
    float height = 40;
    Rectangle inputBounds = {(GetScreenWidth()-width)/2, (GetScreenHeight()-height)/2, width, height};
    // }
}

void DrawEditor(LogicCtx *ctx) {

    Circuit *circ = viewedCircuit;
    if (!circ) return;

    Rectangle editorBounds = {SideBarWidth, 0, GetScreenWidth()-SideBarWidth, GetScreenHeight()};
    Rectangle textBounds = {SideBarWidth+10, 5, 100,20};
    GuiDrawText(circ->name, textBounds, TEXT_ALIGN_LEFT, BLACK);




    Vector2 inCur = {editorBounds.x+50, editorBounds.y + editorBounds.height / 3}; 
    Vector2 outCur = {editorBounds.x+editorBounds.width-50, editorBounds.y + editorBounds.height / 3}; 

    float inOff = (editorBounds.height / 3 - 2*circ->nInPins *ComponentPinRad)/(circ->nInPins+1);
    float outOff = (editorBounds.height / 3 - 2*circ->nOutPins *ComponentPinRad)/(circ->nOutPins+1);
        

    inCur.y += ComponentPinRad + inOff;
    outCur.y += ComponentPinRad + outOff;



    for (int i = 0; i < circ->nWires; i++) {
        Wire *wire = circ->wires + i;
        DrawWire(wire);
    }

    for (int i = 0; i < circ->nComponents; i++) {
        Component *component = circ->components[i];
        DrawComponent(ctx, component, true);
    }

    for (int i = 0; i < circ->nPins; i++) {
        Pin *pin = circ->pins[i];
        if (!pin->component) {
            if (pin->type == OUT) {
                pin->pos = outCur;
                outCur.y += 2*ComponentPinRad + outOff;
            } else if (pin->type == IN) {
                pin->pos = inCur;
                inCur.y += 2*ComponentPinRad + inOff;
            }
            DrawPin(ctx, pin, isEditingRoot());
        }
    }
}




void DrawCircuit(Circuit *circuit, Vector2 pos) {
    const int FontSize = 11;
    const float TextW = MeasureText(circuit->name, FontSize);
    const float NamePad = 8;
    const float Width = TextW + 2 * NamePad;
    const Color BodyColor = BLUE;
    const Color BorderColor = RAYWHITE;
    const Color TextColor = BLACK;

    Rectangle rect = {pos.x-Width/2, pos.y, Width, SideBarCircuitHeight};
    GuiDrawRectangle(rect, 1, BorderColor, BodyColor);
    GuiDrawText(circuit->name, rect, TEXT_ALIGN_MIDDLE, TextColor);
    if (isEditingRoot() && CheckCollisionPointRec(GetMousePosition(), rect)) {
        hovering = true;        
        if (!componentDragging && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            draggedComponent = createComponent(circuit);
            draggedComponent->pos = GetMousePosition();
            componentDragging = true;
        }
    }

}

void DrawComponent(LogicCtx *ctx, Component *component, bool drawPins) {
    const int FontSize = 11;
    const float TextW = MeasureText(component->defn->name, FontSize);
    const float NamePad = 8;
    const Color BodyColor = BLUE;
    const Color BorderColor = RAYWHITE;
    const Color PinColor = GREEN;
    const Color TextColor = BLACK;
    const float Height = fmax(component->nInPins, component->nOutPins) * (2*ComponentPinRad + ComponentPinPad) + ComponentPinPad;
    const float Width = TextW + 2 * (NamePad + ComponentPinRad);

    Vector2 pos = component->pos;
    
    int nInPins = component->nInPins;
    int nOutPins = component->nOutPins;
    float inPinPad = (Height - nInPins * 2 * ComponentPinRad) / (nInPins + 1);
    float outPinPad = (Height - nOutPins * 2 * ComponentPinRad) / (nOutPins + 1);

    Vector2 inPinCur = { pos.x - Width/2, pos.y + Height - inPinPad - ComponentPinRad};
    Vector2 outPinCur = { pos.x + Width/2, pos.y + Height - outPinPad - ComponentPinRad};

    Rectangle rect = {pos.x-Width/2, pos.y, Width, Height};
    GuiDrawRectangle(rect, 1, BorderColor, BodyColor);
    GuiDrawText(component->defn->name, rect, TEXT_ALIGN_MIDDLE, TextColor);

    if (drawPins && !component->defn->eval && CheckCollisionPointRec(GetMousePosition(), rect)) {
        hovering = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
            viewedComponent = component;
            viewedCircuit = component->defn;
            buildingWire = false;
            for (int i = 0; i < component->nPins; i++) {
                setPinState(component->defn->pins[i], component->pins[i]->state);
            }
        }
    }

    if (drawPins) {
        for (int i = component->nPins-1; i >= 0; i--) {
            Pin *pin = component->pins[i];
            if (pin->io) {
                if (pin->type == IN) {
                    pin->pos = inPinCur;
                    inPinCur.y -= inPinPad + 2*ComponentPinRad;
                } else if (pin->type == OUT) {
                    pin->pos = outPinCur;
                    outPinCur.y -= outPinPad + 2*ComponentPinRad;
                }
                DrawPin(ctx, pin, false);
            }
        }
    }
}

void DrawWire(Wire *wire) {
    const float WireThickness = 3;
    
    Vector2 points[wire->pathLen+2];
    points[0] = wire->startPin->pos;
    int nPoints;
    nPoints = wire->pathLen + 1;
    if (wire->endPin) {
        points[nPoints++] = wire->endPin->pos;
    }

    for (int i = 0; i < wire->pathLen; i++) {
        points[i+1] = wire->path[i];
    }

    Color color = wire->startPin->state ? WireColorOn : WireColorOff; 
    for (int i = 1; i < nPoints; i++) {
        DrawLineEx(points[i-1], points[i], WireThickness, color);  
    }

}

void HandleComponentDrag(LogicCtx *ctx) {
    if (componentDragging) {
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (isEditingRoot() && CheckCollisionPointRec(GetMousePosition(), (Rectangle) {SideBarWidth, 0, GetScreenWidth()-SideBarWidth, GetScreenHeight()})) {
                Component *component = addComponent(ctx, viewedCircuit, draggedComponent);
                draggedComponent = NULL;
                componentDragging = false;
            } else {
                deleteComponent(draggedComponent);
                draggedComponent = NULL;
                componentDragging = false;
            } 
        } else {
            draggedComponent->pos = GetMousePosition();
            DrawComponent(ctx, draggedComponent, false);
        }
    }
}

void HandleWireBuild() {
    Rectangle editorRect = {SideBarWidth, 0, GetScreenWidth()-SideBarWidth, GetScreenHeight()}; 
    if (buildingWire) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            buildingWire = false;
        }
        if (!mouseConsumed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), editorRect) && drawnWire.pathLen < PATH_SIZE) {
            drawnWire.pathLen++;
        }
        drawnWire.path[drawnWire.pathLen-1] = GetMousePosition();
        DrawWire(&drawnWire);
    }
}


void DrawPin(LogicCtx *ctx, Pin *pin, bool manualState) {
    Vector2 pos = pin->pos;
    Color color = pin->state ? WireColorOn : WireColorOff;
    DrawCircleV(pos, ComponentPinRad, PinBorderColor);
    DrawCircleV(pos, ComponentPinRad-ComponentPinBorder, color);
    if (manualState  && CheckCollisionPointCircle(GetMousePosition(), pos, ComponentPinRad) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        setPinState(pin, !pin->state);
    }
    if (!isEditingRoot()) {
        return;
    }
    if ((!pin->component && pin->type == OUT) || (pin->component && pin->type == IN)) {
        if (buildingWire) {
            hovering = true;
            if (CheckCollisionPointCircle(GetMousePosition(), pos, ComponentPinRad) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                mouseConsumed = true;

                drawnWire.endPin = pin;
                drawnWire.pathLen--;
                connectPins(viewedCircuit, drawnWire.startPin, pin, &drawnWire);
                buildingWire = false;
            }
        }
    } else if ((!pin->component && pin->type == IN) || (pin->component && pin->type == OUT)){
        if (!buildingWire) {
            hovering = true;
            if (CheckCollisionPointCircle(GetMousePosition(), pos, ComponentPinRad) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                mouseConsumed = true;

                buildingWire = true; 
                drawnWire.startPin  = pin;
                drawnWire.endPin = NULL;
                drawnWire.pathLen = 1;
                drawnWire.path[0] = pos;
            }
        }
    }
}
