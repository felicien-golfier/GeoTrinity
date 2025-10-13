#include "GeoShapes.h"

bool FGeoBox::Overlaps(const FGeoBox& Other) const
{
  return (
      Position.X < Other.Position.X + Other.Size.X &&
      Position.X + Size.X > Other.Position.X &&
      Position.Y < Other.Position.Y + Other.Size.Y &&
      Position.Y + Size.Y > Other.Position.Y
  );
}