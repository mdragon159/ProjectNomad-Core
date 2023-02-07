#pragma once

using uint8 = uint8_t; // At time of writing not sure where Unreal is getting its "uint8" from but hey easy fix
using uint32 = uint32_t;

// Do absolutely nothing for all Unreal-specific macros
#define UENUM(x) 
#define USTRUCT(x) 
#define GENERATED_BODY() 
#define THENOMADGAMEEDITOR_API 
#define UPROPERTY(x) 
// Note that all of the above have one space after the term defined. Pretty sure that's necessary