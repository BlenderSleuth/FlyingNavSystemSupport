// Copyright Ben Sutherland 2021. All rights reserved.

#pragma once

#include "CoreMinimal.h"

// Copied from: Runtime/Engine/Private/DataTableCSV.h for use in shipping builds

class UDataTable;

class FCSVExporter
{
public:
	FCSVExporter(FString& OutExportText);

	bool WriteTable(const UDataTable& InDataTable);

	bool WriteRow(const UScriptStruct* InRowStruct, const void* InRowData, const FProperty* SkipProperty = nullptr);

private:
	bool WriteStructEntry(const void* InRowData, FProperty* InProperty, const void* InPropertyData);

	FString& ExportedText;
};