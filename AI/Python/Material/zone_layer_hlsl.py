"""The zone layer's HLSL, one source for every Custom node of it (make_zone_layer_materials.py pastes these strings).

A slot is three texels of the slot texture UGeoZoneLayerSubsystem writes (World/ZoneLayer/CLAUDE.md, "Slots"): row 0 =
(shape ordinal, colour mask, flags, fill), rows 1-2 = the shape's 8 world-space floats. A new shape is one branch in
SHAPES, matching its case in GeoZoneLayer::PackSlot."""

# Bits of the slot's flags texel: GeoZoneLayer::TelegraphFlag, SafeSpotCutFlag, FillAlongFlag.
TELEGRAPH_FLAG = 1
SAFE_SPOT_CUT_FLAG = 2
FILL_ALONG_FLAG = 4
# Colour bits read from a mask: one per EGeoColor palette slot (static_assert in GeoZoneShape.cpp).
COLOR_BITS = 16
MAX_MEANINGS = 4

# A Custom node's code is a function body: a struct is the one way to declare a function in it.
SHAPES = """struct FGeoZoneShapes
{
    // Signed distance from Position to the shape's outline, negative inside. Core: distance from the shape's core
    // (circle centre, beam middle line); Extent: the core's distance to the outline (radius, half-width). A beam with
    // bFillAlong: Core = distance along it from its start, Extent = its length (EGeoFillMode::AlongBeam).
    float Distance(int Shape, float2 Position, float4 Params0, float4 Params1, bool bFillAlong, out float Extent,
                   out float Core)
    {
        if (Shape == 0)
        {
            // Circle: centre XY, radius.
            Extent = Params0.z;
            Core = length(Position - Params0.xy);
            return Core - Extent;
        }

        // Beam: start XY, end XY; half-width.
        float2 Axis = Params0.zw - Params0.xy;
        float Length = max(length(Axis), 0.0001);
        float2 Forward = Axis / Length;
        float2 Local = Position - Params0.xy;
        float HalfLength = Length * 0.5;
        float Along = dot(Local, Forward);
        float Across = abs(dot(Local, float2(-Forward.y, Forward.x)));
        Extent = bFillAlong ? Length : Params1.x;
        Core = bFillAlong ? Along : Across;
        float2 Outside = float2(abs(Along - HalfLength) - HalfLength, Across - Params1.x);
        return length(max(Outside, 0.0)) + min(max(Outside.x, Outside.y), 0.0);
    }
};
"""

SHAPE_DISTANCE = SHAPES + """FGeoZoneShapes Shapes;
return Shapes.Distance((int)Shape, Position, Params0, Params1, false, Extent, Core);
"""

# Inputs: ZoneSlots (texture), ZoneCount, WorldXY, IsTelegraph, CutMask, OutlineWidth.
# Outputs: return = ColorMask; Counts, Covered, Shade, Outline, OutlineColorIndex.
COVERAGE = SHAPES + f"""FGeoZoneShapes Shapes;
uint Union = 0u;
float BitCounts[{COLOR_BITS}];
for (int Bit = 0; Bit < {COLOR_BITS}; ++Bit)
{{
    BitCounts[Bit] = 0.0;
}}

Covered = 0.0;
Shade = 0.0;
Outline = 0.0;
OutlineColorIndex = 0.0;
bool bDrawsTelegraphs = IsTelegraph > 0.5;
bool bInSafeSpot = CutMask > 0.5;
int Count = (int)ZoneCount;
for (int Slot = 0; Slot < Count; ++Slot)
{{
    float4 Head = ZoneSlots.Load(int3(Slot, 0, 0));
    uint Flags = (uint)Head.z;
    bool bTelegraph = (Flags & {TELEGRAPH_FLAG}u) != 0u;
    bool bCut = (Flags & {SAFE_SPOT_CUT_FLAG}u) != 0u && bInSafeSpot;
    bool bFillAlong = (Flags & {FILL_ALONG_FLAG}u) != 0u;
    if (bTelegraph == bDrawsTelegraphs && !bCut)
    {{
        float Extent;
        float Core;
        float Distance = Shapes.Distance((int)Head.x, WorldXY, ZoneSlots.Load(int3(Slot, 1, 0)),
                                         ZoneSlots.Load(int3(Slot, 2, 0)), bFillAlong, Extent, Core);
        if (Distance <= 0.0)
        {{
            uint Mask = (uint)Head.y;
            Union |= Mask;
            for (int Bit = 0; Bit < {COLOR_BITS}; ++Bit)
            {{
                BitCounts[Bit] += (float)((Mask >> Bit) & 1u);
            }}

            Covered = 1.0;
            // The spent part grows from the core (a FillAlong beam: from its start end): Fill 1 reaches the outline,
            // Fill 0 (EGeoFillMode::None included) shades nothing.
            if (Core < Head.w * Extent)
            {{
                Shade = 1.0;
            }}

            // Each shape keeps its own outline, in its first colour; the last drawn wins where two meet.
            if (Distance >= -OutlineWidth)
            {{
                Outline = 1.0;
                OutlineColorIndex = (float)firstbitlow(Mask);
            }}
        }}
    }}
}}

// How many covering shapes carry each of the first {MAX_MEANINGS} colours, in ordinal order.
Counts = float4(0.0, 0.0, 0.0, 0.0);
int Found = 0;
for (int Bit = 0; Bit < {COLOR_BITS}; ++Bit)
{{
    if (((Union >> Bit) & 1u) != 0u && Found < {MAX_MEANINGS})
    {{
        Counts[Found] = BitCounts[Bit];
        ++Found;
    }}
}}

return (float)Union;
"""

# Inputs: ColorMask, Counts, OutlineColorIndex, Palette (texture), StackBrightness, StackBrightnessMax.
# Outputs: return = ColorCount; Color0-3, OutlineColor.
MASK_COLORS = f"""uint Mask = (uint)ColorMask;
float3 Picked[{MAX_MEANINGS}];
for (int Meaning = 0; Meaning < {MAX_MEANINGS}; ++Meaning)
{{
    Picked[Meaning] = float3(0.0, 0.0, 0.0);
}}

int Found = 0;
for (int Bit = 0; Bit < {COLOR_BITS}; ++Bit)
{{
    if (((Mask >> Bit) & 1u) != 0u && Found < {MAX_MEANINGS})
    {{
        float Brightness = min(1.0 + StackBrightness * (Counts[Found] - 1.0), StackBrightnessMax);
        Picked[Found] = Palette.Load(int3(Bit, 0, 0)).rgb * Brightness;
        ++Found;
    }}
}}

Color0 = Picked[0];
Color1 = Picked[1];
Color2 = Picked[2];
Color3 = Picked[3];
OutlineColor = Palette.Load(int3((int)OutlineColorIndex, 0, 0)).rgb;
return (float)max(Found, 1);
"""
