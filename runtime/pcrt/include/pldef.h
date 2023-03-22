#pragma once

#define constMemb static constexpr auto
#define constMemb_(x) static constexpr x
#define constDef constexpr auto
#define constDef_(x) constexpr x

// Shorts for object declaration and creation
#define _NEWOB(objtype) PlRef<objtype>::createObject()
#define _OB(objtype) PlRef<objtype>

// Short for calling Fmt(). Will be $ in Primal
#define _D(...) primal::Fmt(__VA_ARGS__)
