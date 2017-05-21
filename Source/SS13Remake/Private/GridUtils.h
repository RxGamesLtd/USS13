// The MIT License (MIT)
// Copyright (c) 2017 RxCompile
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
// documentation files (the "Software"), to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
// OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#pragma once

#include "UObject/Object.h"
#include "GridUtils.generated.h"

class AWorldGrid;

USTRUCT(BlueprintType)
struct FGridCell {
	GENERATED_BODY();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FIntVector Index;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallN;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallS;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallE;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 WallW;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	uint8 Floor;

	FGridCell() : WallN(0), WallS(0), WallE(0), WallW(0), Floor(0) {
		Index = FIntVector();
	}

	FGridCell(TSharedPtr<AWorldGrid> grid, FIntVector index)
		: FGridCell() {
		Index = index;
		WorldGrid = grid;
	}

private:

	TSharedPtr<AWorldGrid> WorldGrid;
};
