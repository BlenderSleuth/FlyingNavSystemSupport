// Copyright Ben Sutherland 2021. All rights reserved.

#include "CSVExporter.h"
#include "Engine/DataTable.h"

// Copied from: Runtime/Engine/Private/DataTableCSV.cpp for use in shipping builds

FCSVExporter::FCSVExporter(FString& OutExportText): ExportedText(OutExportText)
{}

bool FCSVExporter::WriteTable(const UDataTable& InDataTable)
{
	if (!InDataTable.RowStruct)
	{
		return false;
	}

	// Write the header (column titles)
	FString ImportKeyField;
	if (!InDataTable.ImportKeyField.IsEmpty())
	{
		// Write actual name if we have it
		ImportKeyField = InDataTable.ImportKeyField;
		ExportedText += ImportKeyField;
	}
	else
	{
	ExportedText += TEXT("---");
	}

	FProperty* SkipProperty = nullptr;
	for (TFieldIterator<FProperty> It(InDataTable.RowStruct); It; ++It)
	{
		FProperty* BaseProp = *It;
		check(BaseProp);

		FString ColumnHeader = DataTableUtils::GetPropertyExportName(BaseProp);
		
		if (ColumnHeader == ImportKeyField)
		{
			// Don't write header again if this is the name field, and save for skipping later
			SkipProperty = BaseProp;
			continue;
		}
		
		ExportedText += TEXT(",");
		ExportedText += ColumnHeader;
	}
	ExportedText += TEXT("\n");

	// Write each row
	for (auto RowIt = InDataTable.GetRowMap().CreateConstIterator(); RowIt; ++RowIt)
	{
		FName RowName = RowIt.Key();
		ExportedText += RowName.ToString();

		uint8* RowData = RowIt.Value();
		WriteRow(InDataTable.RowStruct, RowData);

		ExportedText += TEXT("\n");
	}

	return true;
}

bool FCSVExporter::WriteRow(const UScriptStruct* InRowStruct, const void* InRowData, const FProperty* SkipProperty)
{
	if (!InRowStruct)
	{
		return false;
	}

	for (TFieldIterator<FProperty> It(InRowStruct); It; ++It)
	{
		FProperty* BaseProp = *It;
		check(BaseProp);

		if (BaseProp == SkipProperty)
		{
			continue;
		}

		const void* Data = BaseProp->ContainerPtrToValuePtr<void>(InRowData, 0);
		WriteStructEntry(InRowData, BaseProp, Data);
	}

	return true;
}

bool FCSVExporter::WriteStructEntry(const void* InRowData, FProperty* InProperty, const void* InPropertyData)
{
	ExportedText += TEXT(",");

	const FString PropertyValue = DataTableUtils::GetPropertyValueAsString(InProperty, (uint8*)InRowData, EDataTableExportFlags::None);
	ExportedText += TEXT("\"");
	ExportedText += PropertyValue.Replace(TEXT("\""), TEXT("\"\""));
	ExportedText += TEXT("\"");

	return true;
}