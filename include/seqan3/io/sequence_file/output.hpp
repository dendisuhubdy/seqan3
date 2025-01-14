// -----------------------------------------------------------------------------------------------------
// Copyright (c) 2006-2019, Knut Reinert & Freie Universität Berlin
// Copyright (c) 2016-2019, Knut Reinert & MPI für molekulare Genetik
// This file may be used, modified and/or redistributed under the terms of the 3-clause BSD-License
// shipped with this file and also available at: https://github.com/seqan/seqan3/blob/master/LICENSE.md
// -----------------------------------------------------------------------------------------------------

/*!\file
 * \brief Provides seqan3::sequence_file_output and corresponding traits classes.
 * \author Hannes Hauswedell <hannes.hauswedell AT fu-berlin.de>
 */

#pragma once

#include <cassert>
#include <fstream>
#include <string>
#include <variant>
#include <vector>

// remove the following after range-v3 is updated to 1.0
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <seqan3/core/detail/pack_algorithm.hpp>
#include <seqan3/core/type_list/traits.hpp>
#include <seqan3/core/concept/tuple.hpp>
#include <seqan3/io/stream/concept.hpp>
#include <seqan3/io/exception.hpp>
#include <seqan3/std/filesystem>
#include <seqan3/io/record.hpp>
#include <seqan3/io/detail/misc.hpp>
#include <seqan3/io/detail/misc_output.hpp>
#include <seqan3/io/detail/out_file_iterator.hpp>
#include <seqan3/io/detail/record.hpp>
#include <seqan3/io/sequence_file/format_embl.hpp>
#include <seqan3/io/sequence_file/format_fasta.hpp>
#include <seqan3/io/sequence_file/format_fastq.hpp>
#include <seqan3/io/sequence_file/format_genbank.hpp>
#include <seqan3/io/alignment_file/format_sam.hpp>
#include <seqan3/io/sequence_file/output_format_concept.hpp>
#include <seqan3/io/sequence_file/output_options.hpp>
#include <seqan3/range/views/convert.hpp>
#include <seqan3/range/views/get.hpp>
#include <seqan3/range/views/zip.hpp>
#include <seqan3/std/ranges>

namespace seqan3
{

// ----------------------------------------------------------------------------
// sequence_file_output
// ----------------------------------------------------------------------------

/*!\brief A class for writing sequence files, e.g. FASTA, FASTQ ...
 * \ingroup sequence
 * \tparam selected_field_ids   A seqan3::fields type with the list and order of fields IDs; only relevant if these
 * can't be deduced.
 * \tparam valid_formats        A seqan3::type_list of the selectable formats (each must meet
 * seqan3::sequence_file_output_format).
 * \tparam stream_char_type     The type of the underlying stream device(s); must model seqan3::builtin_character.
 * \details
 *
 * ### Introduction
 *
 * Sequence files are the most generic and common biological files. Well-known formats include
 * FastA and FastQ, but some may also be interested in treating SAM or BAM files as sequence
 * files, discarding the alignment.
 *
 * The Sequence file abstraction supports writing four different fields:
 *
 *   1. seqan3::field::SEQ
 *   2. seqan3::field::ID
 *   3. seqan3::field::QUAL
 *   4. seqan3::field::SEQ_QUAL (sequence and qualities in one range)
 *
 * The member functions take any and either of these fields. If the field ID of an argument cannot be deduced, it
 * is assumed to correspond to the field ID of the respective template parameter.
 *
 * ### Construction and specialisation
 *
 * This class comes with two constructors, one for construction from a file name and one for construction from
 * an existing stream and a known format. The first one automatically picks the format based on the extension
 * of the file name. The second can be used if you have a non-file stream, like std::cout or std::ostringstream,
 * that you want to read from and/or if you cannot use file-extension based detection, but know that your output
 * file has a certain format.
 *
 * In most cases the template parameters are deduced completely automatically:
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_template_deduction.cpp
 *
 * Writing to std::cout:
 * \include test/snippet/io/sequence_file/sequence_file_output_cout_write.cpp
 *
 * Note that this is not the same as writing `sequence_file_output<>` (with angle brackets). In the latter case they are
 * explicitly set to their default values, in the former case
 * [automatic deduction](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction) happens which
 * chooses different parameters depending on the constructor arguments. Prefer deduction over explicit defaults.
 *
 * ### Writing record-wise
 *
 * You can iterate over this file record-wise:
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_record_wise_iteration.cpp
 *
 * The easiest way to write to a sequence file is to use the push_back() or emplace_back() member functions. These
 * work similarly to how they work on an std::vector. If you pass a tuple to push_back() or give arguments to
 * emplace_back() the seqan3::field ID of the i-th tuple-element/argument is assumed to be the i-th value of
 * selected_field_ids, i.e. by default the first is assumed to be seqan3::field::SEQ, the second seqan3::field::ID
 * and the third one seqan3::field::QUAL. You may give less fields than are selected if the actual format you are
 * writing to can cope with less
 * (e.g. for FastA it is sufficient to write seqan3::field::SEQ and seqan3::field::ID, even if selected_field_ids
 * also contains seqan3::field::QUAL at the third position).
 *
 * You may also use the output file's iterator for writing, however, this rarely provides an advantage.
 *
* ### Writing record-wise (custom fields)
 *
 * If you want to pass a combined object for SEQ and QUAL fields to push_back() / emplace_back(), or if you want
 * to change the order of the parameters, you can pass a non-empty fields trait object to the
 * sequence_file_output constructor to select the fields that are used for interpreting the arguments.
 *
 * The following snippets demonstrates the usage of such a fields trait object.
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_fields_trait_1.cpp
 *
 * A different way of passing custom fields to the file is to pass a seqan3::record – instead of a tuple – to
 * push_back(). The seqan3::record clearly indicates which of its elements has which seqan3::field ID so the file will
 * use that information instead of the template argument. This is especially handy when reading from one file and
 * writing to another, because you don't have to configure the output file to match the input file, it will just work:
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_fields_trait_2.cpp
 *
 * ### Writing record-wise in batches
 *
 * You can write multiple records at once, by assigning to the file:
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_batch_write.cpp
 * ### File I/O pipelines
 *
 * Record-wise writing in batches also works for writing from input files directly to output files, because input
 * files are also input ranges in SeqAn:
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_direct_writing.cpp
 *
 * This can be combined with file-based views to create I/O pipelines:
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_view_pipeline.cpp
 *
 * ### Column-based writing
 *
 * The record-based interface treats the file as a range of tuples (the records), but in certain situations
 * you might have the data as columns, i.e. a tuple-of-ranges, instead of range-of-tuples.
 *
 * You can use column-based writing in that case, it uses operator=() :
 *
 * \include test/snippet/io/sequence_file/sequence_file_output_col_based_writing.cpp
 *
 * ### Formats
 *
 * We currently support writing the following formats:
 *   * seqan3::format_fasta
 *   * seqan3::format_fastq
 *   * seqan3::format_embl
 *   * seqan3::format_genbank
 *   * seqan3::format_sam
 */

template <detail::fields_specialisation selected_field_ids_ = fields<field::SEQ, field::ID, field::QUAL>,
          detail::type_list_of_sequence_file_output_formats valid_formats_ =
              type_list<format_embl, format_fasta, format_fastq, format_genbank, format_sam>,
          builtin_character stream_char_type_ = char>
class sequence_file_output
{
public:
    /*!\name Template arguments
     * \brief Exposed as member types for public access.
     * \{
     */
    //!\brief A seqan3::fields list with the fields selected for the record.
    using selected_field_ids    = selected_field_ids_;
    //!\brief A seqan3::type_list with the possible formats.
    using valid_formats         = valid_formats_;
    //!\brief Character type of the stream(s), usually `char`.
    using stream_char_type      = stream_char_type_;
    //!\}

    //!\brief The subset of seqan3::field IDs that are valid for this file.
    using field_ids            = fields<field::SEQ, field::ID, field::QUAL, field::SEQ_QUAL>;

    static_assert([] () constexpr
                  {
                      for (field f : selected_field_ids::as_array)
                          if (!field_ids::contains(f))
                              return false;
                      return true;
                  }(),
                  "You selected a field that is not valid for sequence files, please refer to the documentation "
                  "of sequence_file_output::field_ids for the accepted values.");

    static_assert([] () constexpr
                  {
                      return !(selected_field_ids::contains(field::SEQ_QUAL) &&
                               (selected_field_ids::contains(field::SEQ) ||
                               (selected_field_ids::contains(field::QUAL))));
                  }(),
                  "You may not select field::SEQ_QUAL and either of field::SEQ and field::QUAL at the same time.");

    /*!\name Range associated types
     * \brief Most of the range associated types are `void` for output ranges.
     * \{
     */

    //!\brief The value type (void).
    using value_type        = void;
    //!\brief The reference type (void).
    using reference         = void;
    //!\brief The const reference type (void).
    using const_reference   = void;
    //!\brief The size type (void).
    using size_type         = void;
    //!\brief A signed integer type, usually std::ptrdiff_t.
    using difference_type   = std::ptrdiff_t;
    //!\brief The iterator type of this view (an output iterator).
    using iterator          = detail::out_file_iterator<sequence_file_output>;
    //!\brief The const iterator type is void, because files are not const-iterable.
    using const_iterator    = void;
    //!\brief The type returned by end().
    using sentinel          = std::ranges::default_sentinel_t;
    //!\}

    /*!\name Constructors, destructor and assignment
     * \{
     */
    //!\brief Default constructor is explicitly deleted, you need to give a stream or file name.
    sequence_file_output() = delete;
    //!\brief Copy construction is explicitly deleted, because you can't have multiple access to the same file.
    sequence_file_output(sequence_file_output const &) = delete;
    //!\brief Copy assignment is explicitly deleted, because you can't have multiple access to the same file.
    sequence_file_output & operator=(sequence_file_output const &) = delete;
    //!\brief Move construction is defaulted.
    sequence_file_output(sequence_file_output &&) = default;
    //!\brief Move assignment is defaulted.
    sequence_file_output & operator=(sequence_file_output &&) = default;
    //!\brief Destructor is defaulted.
    ~sequence_file_output() = default;

    /*!\brief Construct from filename.
     * \param[in] filename      Path to the file you wish to open.
     * \param[in] fields_tag    A seqan3::fields tag. [optional]
     *
     * \details
     *
     * In addition to the file name, you may specify a custom seqan3::fields type which may be easier than
     * defining all the template parameters.
     *
     * ### Compression
     *
     * This constructor transparently applies a compression stream on top of the file stream in case
     * the given file extension suggests the user wants this.
     * See the section on \link io_compression compression and decompression \endlink for more information.
     */
    sequence_file_output(std::filesystem::path filename,
                         selected_field_ids const & SEQAN3_DOXYGEN_ONLY(fields_tag) = selected_field_ids{}) :
        primary_stream{new std::ofstream{filename, std::ios_base::out | std::ios::binary}, stream_deleter_default}
    {
        if (!primary_stream->good())
            throw file_open_error{"Could not open file " + filename.string() + " for writing."};

        // possibly add intermediate compression stream
        secondary_stream = detail::make_secondary_ostream(*primary_stream, filename);

        // initialise format handler or throw if format is not found
        detail::set_format(format, filename);
    }

    /*!\brief Construct from an existing stream and with specified format.
     * \tparam file_format   The format of the file in the stream, must satisfy seqan3::sequence_file_output_format.
     * \param[in,out] stream The stream to write to, must be derived of std::basic_ostream<stream_char_t>.
     * \param[in] format_tag The file format tag.
     * \param[in] fields_tag A seqan3::fields tag. [optional]
     *
     * \details
     *
     * ### Compression
     *
     * This constructor **does not** apply compression transparently (because there is no way to know if the user
     * wants this). However, you can just pass e.g. seqan3::contrib::gz_ostream to this constructor if you explicitly
     * want compression.
     * See the section on \link io_compression compression and decompression \endlink for more information.
     */
    template <output_stream stream_t,
              sequence_file_output_format file_format>
    sequence_file_output(stream_t                 & stream,
                         file_format        const & SEQAN3_DOXYGEN_ONLY(format_tag),
                         selected_field_ids const & SEQAN3_DOXYGEN_ONLY(fields_tag) = selected_field_ids{}) :
        primary_stream{&stream, stream_deleter_noop},
        secondary_stream{&stream, stream_deleter_noop},
        format{detail::sequence_file_output_format_exposer<file_format>{}}
    {
        static_assert(list_traits::contains<file_format, valid_formats>,
                      "You selected a format that is not in the valid_formats of this file.");
    }

    //!\overload
    template <output_stream stream_t,
              sequence_file_output_format file_format>
    sequence_file_output(stream_t                && stream,
                         file_format        const & SEQAN3_DOXYGEN_ONLY(format_tag),
                         selected_field_ids const & SEQAN3_DOXYGEN_ONLY(fields_tag) = selected_field_ids{}) :
        primary_stream{new stream_t{std::move(stream)}, stream_deleter_default},
        secondary_stream{&*primary_stream, stream_deleter_noop},
        format{detail::sequence_file_output_format_exposer<file_format>{}}
    {
        static_assert(list_traits::contains<file_format, valid_formats>,
                      "You selected a format that is not in the valid_formats of this file.");
    }
    //!\}

    /*!\name Range interface
     * \brief Provides functions for record based writing of the file.
     * \{
     */
    /*!\brief Returns an iterator to current position in the file.
     * \returns An iterator pointing to the current position in the file.
     *
     * You can write to the file by assigning to the iterator, but using push_back() is usually more intuitive.
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_range_interface.cpp
     */
    iterator begin() noexcept
    {
        return {*this};
    }

    /*!\brief Returns a sentinel for comparison with iterator.
     * \returns An end that is never reached.
     *
     * This element acts as a placeholder; attempting to dereference it results in undefined behaviour. It
     * always compares false against an iterator.
     *
     * ### Complexity
     *
     * Constant.
     *
     * ### Exceptions
     *
     * No-throw guarantee.
     */
    sentinel end() noexcept
    {
        return {};
    }

    /*!\brief           Write a seqan3::record to the file.
     * \tparam record_t Type of the record, a specialisation of seqan3::record.
     * \param[in] r     The record to write.
     *
     * \details
     *
     * ### Complexity
     *
     * Constant. TODO linear in the size of the written sequences?
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_push_back_record.cpp
     */
    template <typename record_t>
    void push_back(record_t && r)
        requires tuple_like<record_t> &&
                 requires { requires detail::is_type_specialisation_of_v<remove_cvref_t<record_t>, record>; }
    {
        write_record(detail::get_or_ignore<field::SEQ>(r),
                     detail::get_or_ignore<field::ID>(r),
                     detail::get_or_ignore<field::QUAL>(r),
                     detail::get_or_ignore<field::SEQ_QUAL>(r));

    }

    /*!\brief           Write a record in form of a std::tuple to the file.
     * \tparam tuple_t  Type of the record, a specialisation of std::tuple.
     * \param[in] t     The record to write.
     *
     * \details
     *
     * The fields in the tuple are assumed to correspond to the field IDs given in selected_field_ids, however
     * passing less is accepted if the format does not require all of them.
     *
     * ### Complexity
     *
     * Constant. TODO linear in the size of the written sequences?
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_push_back_tuple.cpp
     */
    template <typename tuple_t>
    void push_back(tuple_t && t)
        requires tuple_like<tuple_t>
    {
        // index_of might return npos, but this will be handled well by get_or_ignore (and just return ignore)
        write_record(detail::get_or_ignore<selected_field_ids::index_of(field::SEQ)>(t),
                     detail::get_or_ignore<selected_field_ids::index_of(field::ID)>(t),
                     detail::get_or_ignore<selected_field_ids::index_of(field::QUAL)>(t),
                     detail::get_or_ignore<selected_field_ids::index_of(field::SEQ_QUAL)>(t));
    }

    /*!\brief            Write a record to the file by passing individual fields.
     * \tparam arg_t     Type of the first field.
     * \tparam arg_types Types of further fields.
     * \param[in] arg    The first field to write.
     * \param[in] args   Further fields.
     *
     * \details
     *
     * The fields are assumed to correspond to the field IDs given in selected_field_ids, however passing less
     * is accepted if the format does not require all of them.
     *
     * ### Complexity
     *
     * Constant. TODO linear in the size of the written sequences?
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_emplace_back.cpp
     */
    template <typename arg_t, typename ...arg_types>
    void emplace_back(arg_t && arg, arg_types && ... args)
    {
        push_back(std::tie(arg, args...));
    }

    /*!\brief            Write a range of records (or tuples) to the file.
     * \tparam rng_t     Type of the range, must satisfy std::ranges::output_range and have a reference type that
     *                   satisfies seqan3::tuple_like.
     * \param[in] range  The range to write.
     *
     * \details
     *
     * This function simply iterates over the argument and calls push_back() on each element.
     *
     * ### Complexity
     *
     * Linear in the number of records.
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_batch_write.cpp
     */
    template <std::ranges::input_range rng_t>
    sequence_file_output & operator=(rng_t && range)
        requires tuple_like<reference_t<rng_t>>
    {
        for (auto && record : range)
            push_back(std::forward<decltype(record)>(record));
        return *this;
    }

    /*!\brief            Write a range of records (or tuples) to the file.
     * \tparam rng_t     Type of the range, must satisfy std::ranges::input_range and have a reference type that
     *                   satisfies seqan3::tuple_like.
     * \param[in] range  The range to write.
     * \param[in] f      The file being written to.
     *
     * \details
     *
     * This operator enables sequence_file_output to be at the end of a piping operation. It just calls
     * operator=() internally.
     *
     * ### Complexity
     *
     * Linear in the number of records.
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_batch_write.cpp
     *
     * This is especially useful in combination with file-based filters:
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_view_pipeline.cpp
     */
    template <std::ranges::input_range rng_t>
    friend sequence_file_output & operator|(rng_t && range, sequence_file_output & f)
        requires tuple_like<reference_t<rng_t>>
    {
        f = range;
        return f;
    }

    //!\overload
    template <std::ranges::input_range rng_t>
    friend sequence_file_output operator|(rng_t && range, sequence_file_output && f)
        requires tuple_like<reference_t<rng_t>>
    {
    #if defined(__GNUC__) && (__GNUC__ == 9) // an unreported build problem of GCC9
        for (auto && record : range)
            f.push_back(std::forward<decltype(record)>(record));
    #else // ^^^ workaround | regular solution ↓↓↓
        f = range;
    #endif
        return std::move(f);
    }
    //!\}

    /*!\name Tuple interface
     * \brief Provides functions for field-based ("column"-based) writing.
     * \{
     */
    /*!\brief            Write columns (wrapped in a seqan3::record) to the file.
     * \tparam typelist  Template argument to seqan3::record, each type must be a column (range-of-range).
     * \tparam field_ids Template argument to seqan3::record, the IDs corresponding to the columns.
     * \param[in] r      The record of columns.
     *
     * \details
     *
     * \attention This is not part of the row-based file writing; the seqan3::record does not represent a file record,
     * it is a tuple of the columns (with field information).
     *
     * ### Complexity
     *
     * Linear in the size of the columns.
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_col_based_writing.cpp
     */
    template <typename typelist, typename field_ids>
    sequence_file_output & operator=(record<typelist, field_ids> const & r)
    {
        write_columns(detail::range_wrap_ignore(detail::get_or_ignore<field::SEQ>(r)),
                      detail::range_wrap_ignore(detail::get_or_ignore<field::ID>(r)),
                      detail::range_wrap_ignore(detail::get_or_ignore<field::QUAL>(r)),
                      detail::range_wrap_ignore(detail::get_or_ignore<field::SEQ_QUAL>(r)));
        return *this;
    }

    /*!\brief            Write columns (wrapped in a std::tuple) to the file.
     * \tparam arg_types The column types, each type must be a range-of-range.
     * \param[in] t      The tuple of columns.
     *
     * \details
     *
     * The columns are assumed to correspond to the field IDs given in selected_field_ids, however passing less
     * is accepted if the format does not require all of them.
     *
     * ### Complexity
     *
     * Linear in the size of the columns.
     *
     * ### Exceptions
     *
     * Basic exception safety.
     *
     * ### Example
     *
     * \include test/snippet/io/sequence_file/sequence_file_output_col_based_writing.cpp
     */
    template <typename ...arg_types>
    sequence_file_output & operator=(std::tuple<arg_types...> const & t)
    {
        // index_of might return npos, but this will be handled well by get_or_ignore (and just return ignore)
        write_columns(
            detail::range_wrap_ignore(detail::get_or_ignore<selected_field_ids::index_of(field::SEQ)>(t)),
            detail::range_wrap_ignore(detail::get_or_ignore<selected_field_ids::index_of(field::ID)>(t)),
            detail::range_wrap_ignore(detail::get_or_ignore<selected_field_ids::index_of(field::QUAL)>(t)),
            detail::range_wrap_ignore(detail::get_or_ignore<selected_field_ids::index_of(field::SEQ_QUAL)>(t)));
        return *this;
    }
    //!\}

    //!\brief The options are public and its members can be set directly.
    sequence_file_output_options options{};

    /*!\cond DEV
     * \brief Expose a reference to the secondary stream. [public, but not documented as part of the API]
     */
    std::basic_ostream<stream_char_type> & get_stream()
    {
        return *secondary_stream;
    }
    //!\endcond
protected:
    //!\privatesection

    /*!\name Stream / file access
     * \{
     */
    //!\brief The type of the internal stream pointers. Allows dynamically setting ownership management.
    using stream_ptr_t = std::unique_ptr<std::basic_ostream<stream_char_type>,
                                         std::function<void(std::basic_ostream<stream_char_type>*)>>;
    //!\brief Stream deleter that does nothing (no ownership assumed).
    static void stream_deleter_noop(std::basic_ostream<stream_char_type> *) {}
    //!\brief Stream deleter with default behaviour (ownership assumed).
    static void stream_deleter_default(std::basic_ostream<stream_char_type> * ptr) { delete ptr; }

    //!\brief The primary stream is the user provided stream or the file stream if constructed from filename.
    stream_ptr_t primary_stream{nullptr, stream_deleter_noop};
    //!\brief The secondary stream is a compression layer on the primary or just points to the primary (no compression).
    stream_ptr_t secondary_stream{nullptr, stream_deleter_noop};

    //!\brief Type of the format, an std::variant over the `valid_formats`.
    using format_type = typename detail::variant_from_tags<valid_formats,
                                                           detail::sequence_file_output_format_exposer>::type;
    //!\brief The actual std::variant holding a pointer to the detected/selected format.
    format_type format;
    //!\}

    //!\brief Write record to format.
    template <typename seq_t, typename id_t, typename qual_t, typename seq_qual_t>
    void write_record(seq_t && seq, id_t && id, qual_t && qual, seq_qual_t && seq_qual)
    {
        static_assert(detail::decays_to_ignore_v<seq_qual_t> ||
                      (detail::decays_to_ignore_v<seq_t> && detail::decays_to_ignore_v<qual_t>),
                      "You may not select field::SEQ_QUAL and either of field::SEQ and field::QUAL at the same time.");

        if constexpr (!detail::decays_to_ignore_v<seq_qual_t>)
            static_assert(detail::is_type_specialisation_of_v<value_type_t<seq_qual_t>, qualified>,
                          "The SEQ_QUAL field must contain a range over the seqan3::qualified alphabet.");

        assert(!format.valueless_by_exception());
        std::visit([&] (auto & f)
        {
            if constexpr (!detail::decays_to_ignore_v<seq_qual_t>)
            {
                f.write_sequence_record(*secondary_stream,
                                        options,
                                        seq_qual | views::get<0>,
                                        id,
                                        seq_qual | views::get<1>);
            }
            else
            {
                f.write_sequence_record(*secondary_stream,
                                        options,
                                        seq,
                                        id,
                                        qual);
            }
        }, format);
    }

    //!\brief Write columns to file format, only tag-dispatch once.
    template <std::ranges::input_range seqs_t,
              std::ranges::input_range ids_t,
              std::ranges::input_range quals_t,
              std::ranges::input_range seq_quals_t>
    void write_columns(seqs_t       && seqs,
                       ids_t        && ids,
                       quals_t      && quals,
                       seq_quals_t  && seq_quals)
    {
        static_assert(!(detail::decays_to_ignore_v<reference_t<seqs_t>> &&
                        detail::decays_to_ignore_v<reference_t<ids_t>> &&
                        detail::decays_to_ignore_v<reference_t<quals_t>> &&
                        detail::decays_to_ignore_v<reference_t<seq_quals_t>>),
                      "At least one of the columns must not be set to std::ignore.");

        static_assert(detail::decays_to_ignore_v<reference_t<seq_quals_t>> ||
                      (detail::decays_to_ignore_v<reference_t<seqs_t>> &&
                       detail::decays_to_ignore_v<reference_t<quals_t>>),
                      "You may not select field::SEQ_QUAL and either of field::SEQ and field::QUAL at the same time.");

        if constexpr (!detail::decays_to_ignore_v<reference_t<seq_quals_t>>)
            static_assert(detail::is_type_specialisation_of_v<value_type_t<reference_t<seq_quals_t>>, qualified>,
                          "The SEQ_QUAL field must contain a range over the seqan3::qualified alphabet.");

        assert(!format.valueless_by_exception());
        std::visit([&] (auto & f)
        {
            if constexpr (!detail::decays_to_ignore_v<reference_t<seq_quals_t>>)
            {
                auto zipped = views::zip(seq_quals, ids);

                for (auto && v : zipped)
                    f.write_sequence_record(*secondary_stream,
                                            options,
                                            std::get<0>(v) | views::get<0>,
                                            std::get<1>(v),
                                            std::get<0>(v) | views::get<1>);
            }
            else
            {
                auto zipped = views::zip(seqs, ids, quals);

                for (auto && v : zipped)
                    f.write_sequence_record(*secondary_stream, options, std::get<0>(v), std::get<1>(v), std::get<2>(v));
            }
        }, format);
    }

    //!\brief Befriend iterator so it can access the buffers.
    friend iterator;
};

/*!\name Type deduction guides
 * \relates seqan3::sequence_file_output
 * \{
 */

//!\brief Deduction guide for given stream and file format.
template <output_stream                 stream_t,
          sequence_file_output_format file_format>
sequence_file_output(stream_t &,
                     file_format const &)
    -> sequence_file_output<typename sequence_file_output<>::selected_field_ids,  // default field ids
                            type_list<file_format>,
                            typename std::remove_reference_t<stream_t>::char_type>;

//!\overload
template <output_stream                 stream_t,
          sequence_file_output_format file_format>
sequence_file_output(stream_t &&,
                     file_format const &)
    -> sequence_file_output<typename sequence_file_output<>::selected_field_ids,  // default field ids.
                            type_list<file_format>,
                            typename std::remove_reference_t<stream_t>::char_type>;

//!\brief Deduction guide for given stream, file format and field ids.
template <output_stream                 stream_t,
          sequence_file_output_format file_format,
          detail::fields_specialisation           selected_field_ids>
sequence_file_output(stream_t &&,
                     file_format const &,
                     selected_field_ids const &)
    -> sequence_file_output<selected_field_ids,
                            type_list<file_format>,
                            typename std::remove_reference_t<stream_t>::char_type>;

//!\overload
template <output_stream                 stream_t,
          sequence_file_output_format file_format,
          detail::fields_specialisation           selected_field_ids>
sequence_file_output(stream_t &,
                     file_format const &,
                     selected_field_ids const &)
    -> sequence_file_output<selected_field_ids,
                            type_list<file_format>,
                            typename std::remove_reference_t<stream_t>::char_type>;
//!\}
} // namespace seqan3

#pragma GCC diagnostic pop
