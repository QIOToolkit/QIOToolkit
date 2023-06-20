// Copyright (c) Microsoft. All Rights Reserved.
#pragma once

#ifdef DEBUG_SELFCONSISTENCY

#define self_consistency_assert(check) assert(check);

#else

#define self_consistency_assert(check) ;

#endif
