/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
 * Copyright (c) 2023-2024 Epic Games, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef PAS_COMMITTED_PAGES_VECTOR_H
#define PAS_COMMITTED_PAGES_VECTOR_H

#include "pas_log.h"
#include "pas_utils.h"

#ifndef _WIN32
#include <sys/types.h>
#include <sys/mman.h>
#endif

PAS_BEGIN_EXTERN_C;

/* PAS_COMMITTED_PAGES_VECTOR_WORKS tells us if this really counts committed pages the way we like.
   
   On Windows, this will count some decommitted pages as committed, if they are decommitted using the asymmetric (pas_may_not_mmap)
   style. */
#ifdef _WIN32
#define PAS_COMMITTED_PAGES_VECTOR_WORKS 0
#else
#define PAS_COMMITTED_PAGES_VECTOR_WORKS 1
#endif

struct pas_allocation_config;
struct pas_committed_pages_vector;
typedef struct pas_allocation_config pas_allocation_config;
typedef struct pas_committed_pages_vector pas_committed_pages_vector;

struct pas_committed_pages_vector {
    char* raw_data;
    size_t size;
};

PAS_API void pas_committed_pages_vector_construct(pas_committed_pages_vector* vector,
                                                  void* object,
                                                  size_t size,
                                                  const pas_allocation_config* allocation_config);

PAS_API void pas_committed_pages_vector_destruct(pas_committed_pages_vector* vector,
                                                 const pas_allocation_config* allocation_config);

static inline bool pas_committed_pages_vector_is_committed(pas_committed_pages_vector* vector,
                                                           size_t page_index)
{
    static const bool verbose = false;
    PAS_ASSERT(page_index < vector->size);
    if (verbose) {
        pas_log("vector->raw_data[%zu] = %u\n",
                page_index, (unsigned)(unsigned char)vector->raw_data[page_index]);
    }
#if PAS_OS(LINUX) || defined(_WIN32)
    return vector->raw_data[page_index];
#else
    return vector->raw_data[page_index] & (MINCORE_REFERENCED |
                                           MINCORE_REFERENCED_OTHER |
                                           MINCORE_MODIFIED_OTHER |
                                           MINCORE_MODIFIED);
#endif
}

PAS_API size_t pas_committed_pages_vector_count_committed(pas_committed_pages_vector* vector);

PAS_API size_t pas_count_committed_pages(void* object,
                                         size_t size,
                                         const pas_allocation_config* allocation_config);

PAS_END_EXTERN_C;

#endif /* PAS_COMMITTED_PAGES_VECTOR_H */

