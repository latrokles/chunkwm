#include "region.h"
#include "node.h"
#include "vspace.h"
#include "cvar.h"

#include "../../common/misc/assert.h"
#include "../../common/accessibility/display.h"

#include <stdio.h>

#define internal static

internal region
FullscreenRegion(macos_display *Display)
{
    macos_space *Space = AXLibActiveSpace(Display->Ref);
    ASSERT(Space);

    virtual_space *VirtualSpace = AcquireVirtualSpace(Display, Space);
    region_offset *Offset = &VirtualSpace->Offset;

    region Result;

    Result.X = Display->X + Offset->Left;
    Result.Y = Display->Y + Offset->Top;
    Result.Width = Display->Width - Offset->Left - Offset->Right;
    Result.Height = Display->Height - Offset->Top - Offset->Bottom;

    AXLibDestroySpace(Space);
    return Result;
}

internal region
LeftVerticalRegion(macos_display *Display, node *Node)
{
    macos_space *Space = AXLibActiveSpace(Display->Ref);

    ASSERT(Space);
    ASSERT(Node);

    virtual_space *VirtualSpace = AcquireVirtualSpace(Display, Space);
    region_offset *Offset = &VirtualSpace->Offset;

    region *Region = &Node->Region;
    region Result;

    Result.X = Region->X;
    Result.Y = Region->Y;

    Result.Width = (Region->Width * Node->Ratio) - (Offset->Gap / 2);
    Result.Height = Region->Height;

    AXLibDestroySpace(Space);
    return Result;
}

internal region
RightVerticalRegion(macos_display *Display, node *Node)
{
    macos_space *Space = AXLibActiveSpace(Display->Ref);

    ASSERT(Space);
    ASSERT(Node);

    virtual_space *VirtualSpace = AcquireVirtualSpace(Display, Space);
    region_offset *Offset = &VirtualSpace->Offset;

    region *Region = &Node->Region;
    region Result;

    Result.X = Region->X + (Region->Width * Node->Ratio) + (Offset->Gap / 2);
    Result.Y = Region->Y;

    Result.Width = (Region->Width * (1 - Node->Ratio)) - (Offset->Gap / 2);
    Result.Height = Region->Height;

    AXLibDestroySpace(Space);
    return Result;
}

internal region
UpperHorizontalRegion(macos_display *Display, node *Node)
{
    macos_space *Space = AXLibActiveSpace(Display->Ref);

    ASSERT(Space);
    ASSERT(Node);

    virtual_space *VirtualSpace = AcquireVirtualSpace(Display, Space);
    region_offset *Offset = &VirtualSpace->Offset;

    region *Region = &Node->Region;
    region Result;

    Result.X = Region->X;
    Result.Y = Region->Y;

    Result.Width = Region->Width;
    Result.Height = (Region->Height * Node->Ratio) - (Offset->Gap / 2);

    AXLibDestroySpace(Space);
    return Result;
}

internal region
LowerHorizontalRegion(macos_display *Display, node *Node)
{
    macos_space *Space = AXLibActiveSpace(Display->Ref);

    ASSERT(Space);
    ASSERT(Node);

    virtual_space *VirtualSpace = AcquireVirtualSpace(Display, Space);
    region_offset *Offset = &VirtualSpace->Offset;

    region *Region = &Node->Region;
    region Result;

    Result.X = Region->X;
    Result.Y = Region->Y + (Region->Height * Node->Ratio) + (Offset->Gap / 2);

    Result.Width = Region->Width;
    Result.Height = (Region->Height * (1 - Node->Ratio)) - (Offset->Gap / 2);

    AXLibDestroySpace(Space);
    return Result;
}

void CreateNodeRegion(macos_display *Display, node *Node, region_type Type)
{
    if(Node->Ratio == 0)
    {
        Node->Ratio = CVarFloatingPointValue("bsp_split_ratio");
    }

    ASSERT(Type >= Region_Full && Type <= Region_Lower);
    switch(Type)
    {
        case Region_Full:
        {
            Node->Region = FullscreenRegion(Display);
        } break;
        case Region_Left:
        {
            Node->Region = LeftVerticalRegion(Display, Node->Parent);
        } break;
        case Region_Right:
        {
            Node->Region = RightVerticalRegion(Display, Node->Parent);
        } break;
        case Region_Upper:
        {
            Node->Region = UpperHorizontalRegion(Display, Node->Parent);
        } break;
        case Region_Lower:
        {
            Node->Region = LowerHorizontalRegion(Display, Node->Parent);
        } break;
        default: { /* NOTE(koekeishiya): Invalid region specified. */} break;
    }

    if(Node->Split == Split_None)
    {
        Node->Split = OptimalSplitMode(Node);
    }

    Node->Region.Type = Type;
}

internal void
CreateNodeRegionPair(macos_display *Display, node *Left, node *Right, node_split Split)
{
    ASSERT(Split == Split_Vertical || Split == Split_Horizontal);
    if(Split == Split_Vertical)
    {
        CreateNodeRegion(Display, Left, Region_Left);
        CreateNodeRegion(Display, Right, Region_Right);
    }
    else if (Split == Split_Horizontal)
    {
        CreateNodeRegion(Display, Left, Region_Upper);
        CreateNodeRegion(Display, Right, Region_Lower);
    }
}

void ResizeNodeRegion(macos_display *Display, node *Node)
{
    if(Node)
    {
        if(Node->Left)
        {
            CreateNodeRegion(Display, Node->Left, Node->Left->Region.Type);
            ResizeNodeRegion(Display, Node->Left);
        }

        if(Node->Right)
        {
            CreateNodeRegion(Display, Node->Right, Node->Right->Region.Type);
            ResizeNodeRegion(Display, Node->Right);
        }
    }
}

void CreateNodeRegionRecursive(macos_display *Display, node *Node, bool Optimal)
{
    if(Node && Node->Left && Node->Right)
    {
        Node->Split = Optimal ? OptimalSplitMode(Node) : Node->Split;
        CreateNodeRegionPair(Display, Node->Left, Node->Right, Node->Split);

        CreateNodeRegionRecursive(Display, Node->Left, Optimal);
        CreateNodeRegionRecursive(Display, Node->Right, Optimal);
    }
}
