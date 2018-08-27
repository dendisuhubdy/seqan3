// ============================================================================
//                 SeqAn - The Library for Sequence Analysis
// ============================================================================
//
// Copyright (c) 2006-2018, Knut Reinert & Freie Universitaet Berlin
// Copyright (c) 2016-2018, Knut Reinert & MPI Molekulare Genetik
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Knut Reinert or the FU Berlin nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL KNUT REINERT OR THE FU BERLIN BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
//
// ============================================================================

/*!\file
 * \brief Provides seqan3::align::gap_linear.
 * \author Rene Rahn <rene.rahn AT fu-berlin.de>
 * \author Jörg Winkler <j.winkler AT fu-berlin.de>
 */

#pragma once

#include <seqan3/alignment/gap/detail.hpp>
#include <seqan3/alignment/scoring/gap_scheme.hpp>
#include <seqan3/core/detail/strong_type.hpp>

namespace seqan3
{

/*!\brief Data structure for linear gaps
 * \ingroup alignment
 * \tparam value_type The value type for the gap score.
 */
template <arithmetic_concept value_type>
struct gap_linear
{

    /*!\name Constructor, destructor and assignment
     * \{
     */
    gap_linear()                               = default;
    gap_linear(gap_linear const &)             = default;
    gap_linear(gap_linear &&)                  = default;
    gap_linear & operator=(gap_linear const &) = default;
    gap_linear & operator=(gap_linear &&)      = default;
    ~gap_linear()                              = default;

    //!\brief Construction from seqan3::gap_score.
    template <arithmetic_concept inner_value_type>
    constexpr gap_linear(gap_score<inner_value_type> const gs) noexcept : scheme{std::move(gs)}
    {}
    //!}

    //!\privatesection
    //!\brief The data member storing the score.
    gap_scheme<value_type> scheme;
};

/*!\name Deduction guides
 * \brief Deduces template parameter from the argument.
 * \relates seqan3::gap_linear
 * \{
 */
template <arithmetic_concept value_type>
gap_linear(gap_score<value_type>) -> gap_linear<value_type>;
//!\}

} // namespace seqan3

namespace seqan3::detail
{

//!\cond
template <arithmetic_concept value_type>
struct is_gap_config<gap_linear<value_type>> : public std::true_type
{};
//!\endcond

} // namespace seqan3::detail
