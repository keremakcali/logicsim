#include "logic.h"
#include "raylib.h"
#include "stdio.h"

#include <raylib.h>
#include <string.h>


#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "config.h"

void DrawControls(LogicCtx *ctx);
void DrawEditor(LogicCtx *ctx);

void DrawComponent(LogicCtx *ctx, Component *component);

bool nameInput = false;

int main() {

    readConfig();

    LogicCtx *ctx = logicInit();


    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Logic Sim");
    SetTargetFPS(60);

    ctx->viewedCircuit = createCircuit("NEW CIRCUIT", NULL);
    ctx->componentDragging = false;
    ctx->hovering = false;

    while (!WindowShouldClose()) {

        readConfig();

        if (ctx->hovering) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        } else {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
        }
        ctx->hovering = false;

        BeginDrawing();

        bool active = true;

        DrawControls(ctx);

        DrawEditor(ctx);

        if (ctx->componentDragging) {
            DrawRectangleV(GetMousePosition(), ctx->draggedComponent->dimentions, BLUE);    
        }

        char text[20];
        GuiTextBox((Rectangle) {100,100,200,100}, text, 20, true);
        EndDrawing();
    }

    CloseWindow();

    logicFree(ctx);
}



void DrawControls(LogicCtx *ctx) {
    bool hovering = false;
    bool input = false;
    Rectangle panelBounds = {0, 0, SideBarWidth, GetScreenHeight()-2*SideBarButtonHeight};

    Rectangle newButtBounds = {0,GetScreenHeight()-SideBarButtonHeight, SideBarWidth, SideBarButtonHeight}; 
    Rectangle newInPinBounds = {0,GetScreenHeight()-2*SideBarButtonHeight, SideBarWidth/2, SideBarButtonHeight};
    Rectangle newOutPinBounds = {SideBarWidth/2,GetScreenHeight()-2*SideBarButtonHeight, SideBarWidth/2, SideBarButtonHeight};

    Rectangle inputBounds = {(GetScreenWidth()-SideBarWidth)/2, (GetScreenHeight()-SideBarButtonHeight)/2,SideBarWidth,SideBarButtonHeight}; 
    static char inputStr[30];

    Rectangle content = {0, 0, SideBarWidth - SCROLLBAR_WIDTH, SideBarComponentPadding} ;
    static Vector2 scroll = {0, 0};
    Rectangle view = {0};

    if (CheckCollisionPointRec(GetMousePosition(), panelBounds)) {
        if (ctx->componentDragging && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
            ctx->componentDragging = false;
        }
    }

    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        Circuit *circuit = ctx->circuits[i];
        Vector2 dim = circuit->dimentions;
        content.height += dim.y + SideBarComponentPadding;
    }

    GuiScrollPanel(panelBounds, "Components", content, &scroll, &view);

    Vector2 cur = {(content.width- SideBarComponentWidth)/2, 30};
    BeginScissorMode(view.x, view.y, view.width, view.height);
    for (int i = ctx->nCircuits-1; i >= 0; i--) {
        Circuit *circuit = ctx->circuits[i];
        Vector2 dim = circuit->dimentions;
        dim.x = SideBarComponentWidth;
        Vector2 recPos = {cur.x + scroll.x, cur.y + scroll.y};
        DrawRectangleV(recPos, dim, BLUE);
        if (CheckCollisionPointRec(GetMousePosition(), (Rectangle) {recPos.x, recPos.y, dim.x, dim.y})) {
            ctx->hovering = true;
            if (!ctx->componentDragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                ctx->componentDragging = true;
                ctx->draggedComponent = circuit;
            }
        }
        cur.y += dim.y + SideBarComponentPadding;
    }
    EndScissorMode();

    if (nameInput) {
        float width = 100;
        float height = 40;
        Rectangle inputBounds = {(GetScreenWidth()-width)/2, (GetScreenHeight()-height)/2, width, height};
        GuiTextBox(inputBounds, inputStr, 30, true);
    }

    if (GuiButton(newButtBounds, "New Circuit")) {
        strcpy(ctx->viewedCircuit->name, inputStr);
        addCircuit(ctx, ctx->viewedCircuit, NULL);
        ctx->viewedCircuit = createCircuit("NEW CIRCUIT", NULL);
        inputStr[0] = 0;
    }

    if (GuiButton(newInPinBounds, "New In Pin")) {
        newPin(ctx, ctx->viewedCircuit, inputStr, IN);
        inputStr[0] = 0;
    }
    if (GuiButton(newOutPinBounds, "New Out Pin")) {
        newPin(ctx, ctx->viewedCircuit, inputStr, OUT);
        inputStr[0] = 0;
    }
}

void DrawEditor(LogicCtx *ctx) {

    Circuit *circ = ctx->viewedCircuit;
    if (!circ) return;

    Rectangle editorBounds = {SideBarWidth, 0, GetScreenWidth()-SideBarWidth, GetScreenHeight()};
    GuiDrawRectangle(editorBounds, 0, WHITE, DARKGRAY);
    Rectangle textBounds = {SideBarWidth+10, 5, 100,20};
    GuiDrawText(circ->name, textBounds, TEXT_ALIGN_LEFT, BLACK);




    Vector2 inCur = {editorBounds.x+50, editorBounds.y + editorBounds.height / 3}; 
    Vector2 outCur = {editorBounds.x+editorBounds.width-50, editorBounds.y + editorBounds.height / 3}; 

    float inOff = (editorBounds.height / 3 - 2*circ->nInPins *ComponentPinRad)/(circ->nInPins+1);
        
    float outOff = (editorBounds.height / 3 - 2*circ->nOutPins *ComponentPinRad)/(circ->nOutPins+1);

    inCur.y += ComponentPinRad + inOff;
    outCur.y += ComponentPinRad + outOff;


    if (CheckCollisionPointRec(GetMousePosition(), editorBounds)) {
        if (ctx->componentDragging && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
            Component *newComponent = addComponent(ctx, ctx->viewedCircuit, ctx->draggedComponent);
            printf("%p\n", newComponent);
            newComponent->pos = GetMousePosition();
            ctx->componentDragging = false;
            ctx->draggedComponent = NULL;
        }
    }

    if (ctx->buildingWire) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ctx->wirePath[ctx->pathLen++] = GetMousePosition();
        }
        if (ctx->pathLen < 1) {
            DrawLineBezier(ctx->wireSrcPin->pos, GetMousePosition(), 3, RED);
        } else {
            DrawLineBezier(ctx->wireSrcPin->pos, ctx->wirePath[0], 3, RED);
            for (int i = 1; i < ctx->pathLen; i++) {
                DrawLineBezier(ctx->wirePath[i-1], ctx->wirePath[i], 3, RED);
            }
            DrawLineBezier(ctx->wirePath[ctx->pathLen-1], GetMousePosition(), 3, RED);
        }
    }

    for (int i = 0; i < circ->nWires; i++) {
        Wire *wire = circ->wires + i;
        if (wire->pathLen < 1) {
            DrawLineBezier(*wire->pathStart, *wire->pathEnd, 3, RED);
        } else {
            DrawLineBezier(*wire->pathStart, wire->path[0], 3, RED);
            for (int i = 1; i < wire->pathLen; i++) {
                DrawLineBezier(wire->path[i-1], wire->path[i], 3, RED); 
            }
            DrawLineBezier(wire->path[wire->pathLen-1], *wire->pathEnd, 3, RED);
        }
    }

    for (int i = 0; i < circ->nComponents; i++) {
        Component *component = circ->components[i];
        DrawComponent(ctx, component);
        GuiDrawText(component->defn->name, (Rectangle) {component->pos.x, component->pos.y, component->defn->dimentions.x,component->defn->dimentions.y} , TEXT_ALIGN_CENTER, BLACK);
    }

    for (int i = 0; i < circ->nPins; i++) {
        Pin *pin = circ->pins[i];
        if (pin->type == IN) {
            DrawCircleV(inCur, ComponentPinRad, RAYWHITE);
            pin->pos = inCur;
            if (!ctx->buildingWire && CheckCollisionPointCircle(GetMousePosition(), inCur, ComponentPinRad)) {
                ctx->hovering = true;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ctx->buildingWire = true;
                    ctx->pathLen = 0;
                    ctx->wireSrcPin = pin;
                }
            }
            inCur.y += 2*ComponentPinRad + inOff;
        } else {
            DrawCircleV(outCur, ComponentPinRad, RAYWHITE);
            pin->pos = outCur;

            if (ctx->buildingWire && CheckCollisionPointCircle(GetMousePosition(), outCur, ComponentPinRad)) {
                ctx->hovering = true;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ctx->buildingWire = false;
                    ctx->wireDestPin = pin;
                    ctx->pathLen--;
                    connectPins(ctx, ctx->wireSrcPin, pin);
                }
            }

            outCur.y += 2*ComponentPinRad + outOff;
        }
    }

}



void DrawComponent(LogicCtx *ctx, Component *component) {
    Rectangle compRec = {component->pos.x, component->pos.y, component->defn->dimentions.x, component->defn->dimentions.y};
    GuiDrawRectangle(compRec, 1, RAYWHITE, BLUE);

    float w = component->defn->dimentions.x;
    float h = component->defn->dimentions.y;

    int nIn = component->nInPins;
    int nOut = component->nOutPins;

    int inOff = (h - nIn*2*ComponentPinRad) / (nIn+1);
    int outOff = (h - nOut*2*ComponentPinRad) / (nOut+1);

    Vector2 inCur = {component->pos.x, component->pos.y+inOff+ComponentPinRad};
    Vector2 outCur = {component->pos.x+w, component->pos.y+outOff+ComponentPinRad};

    for (int i = 0; i < component->nPins; i++) {
        Pin *pin = component->pins[i];
        if (pin->type == IN) {
            DrawCircleV(inCur, ComponentPinRad, GREEN);
            pin->pos = inCur;

            if (ctx->buildingWire && CheckCollisionPointCircle(GetMousePosition(), inCur, ComponentPinRad)) {
                ctx->hovering = true;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ctx->buildingWire = false;
                    ctx->wireDestPin = pin;
                    ctx->pathLen--;
                    connectPins(ctx, ctx->wireSrcPin, pin);
                }
            }
            inCur.y += inOff + 2*ComponentPinRad;
        } else {
            DrawCircleV(outCur, ComponentPinRad, GREEN);
            pin->pos = outCur;
            if (!ctx->buildingWire && CheckCollisionPointCircle(GetMousePosition(), outCur, ComponentPinRad)) {
                ctx->hovering = true;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ctx->buildingWire = true;
                    ctx->pathLen = 0;
                    ctx->wireSrcPin = pin;
                }
            }
            outCur.y += outOff + 2*ComponentPinRad;
        } 
    }

}

