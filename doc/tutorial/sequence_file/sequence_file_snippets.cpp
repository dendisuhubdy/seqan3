#include <iostream>
#include <fstream>

#include <range/v3/numeric/accumulate.hpp>   // ranges::accumulate
//![include_ranges_chunk]
#include <range/v3/view/chunk.hpp>
//![include_ranges_chunk]

#include <seqan3/core/char_operations/predicate.hpp>
//![include]
#include <seqan3/io/sequence_file/all.hpp>
//![include]
//![include_debug_stream]
#include <seqan3/core/debug_stream.hpp>
//![include_debug_stream]
#include <seqan3/range/detail/misc.hpp>
//![include_ranges]
#include <seqan3/std/ranges>
//![include_ranges]

struct write_file_dummy_struct
{
    std::filesystem::path const tmp_path = std::filesystem::temp_directory_path();

    write_file_dummy_struct()
    {

auto file_raw = R"//![fastq_file](
@seq1
AGCTAGCAGCGATCG
+
IIIIIHIIIIIIIII
@seq2
CGATCGATC
+
IIIIIIIII
@seq3
AGCGATCGAGGAATATAT
+
IIIIHHGIIIIHHGIIIH
)//![fastq_file]";

        std::ofstream file{tmp_path/"my.fastq"};
        std::string str{file_raw};
        file << str.substr(1); // skip first newline

        std::ofstream file2{tmp_path/"my.qq"};
        file2 << str.substr(1); // skip first newline

        std::ofstream file3{tmp_path/"my.fasta"};
        file3 << ">seq1\nAVAV\n>seq2\nAVAVA\n";
    }

    ~write_file_dummy_struct()
    {
        std::error_code ec{};
        std::filesystem::path file_path{};

        file_path = tmp_path/"my.fastq";
        std::filesystem::remove(file_path, ec);
        if (ec)
            seqan3::debug_stream << "[WARNING] Could not delete " << file_path << ". " << ec.message() << '\n';

        file_path = tmp_path/"my.qq";
        std::filesystem::remove(file_path, ec);
        if (ec)
            seqan3::debug_stream << "[WARNING] Could not delete " << file_path << ". " << ec.message() << '\n';

        file_path = tmp_path/"my.fasta";
        std::filesystem::remove(file_path, ec);
        if (ec)
            seqan3::debug_stream << "[WARNING] Could not delete " << file_path << ". " << ec.message() << '\n';
    }
};

write_file_dummy_struct go{}; // write file

using namespace seqan3;

int main()
{

{
//![file_extensions]
debug_stream << format_fastq::file_extensions << std::endl; // prints [fastq,fq]
//![file_extensions]

//![modify_file_extensions]
format_fastq::file_extensions.push_back("qq");
sequence_file_input fin{std::filesystem::temp_directory_path()/"my.qq"}; // detects FASTQ format
//![modify_file_extensions]
}

{
/*
//![construct_from_cin]
sequence_file_input fin{std::cin, format_fasta{}};
//![construct_from_cin]
*/
}

{
//![amino_acid_type_trait]
sequence_file_input<sequence_file_input_default_traits_aa> fin{std::filesystem::temp_directory_path()/"my.fasta"};
//![amino_acid_type_trait]
}

{
//![record_type]
sequence_file_input fin{std::filesystem::temp_directory_path()/"my.fastq"};
using record_type = typename decltype(fin)::record_type;

// Because `fin` is a range, we can access the first element by dereferencing fin.begin()
record_type rec = *fin.begin();
//![record_type]
}

{
sequence_file_input fin{std::filesystem::temp_directory_path()/"my.fastq"};
using record_type = typename decltype(fin)::record_type;
//![record_type2]
record_type rec = std::move(*fin.begin()); // avoid copying
//![record_type2]
}

{
//![paired_reads]
sequence_file_input fin1{std::filesystem::temp_directory_path()/"my.fastq"};
sequence_file_input fin2{std::filesystem::temp_directory_path()/"my.fastq"}; // for simplicity we take the same file

for (auto && [rec1, rec2] : views::zip(fin1, fin2)) // && is important! because views::zip returns temporaries
{
    if (get<field::ID>(rec1) != get<field::ID>(rec2))
        throw std::runtime_error("Oh oh your pairs don't match.");
}
//![paired_reads]
}

{
//![read_in_batches]
sequence_file_input fin{std::filesystem::temp_directory_path()/"my.fastq"};

for (auto && records : fin | ranges::view::chunk(10)) // && is important! because views::chunk returns temporaries
{
    // `records` contains 10 elements (or less at the end)
    debug_stream << "Taking the next 10 sequences:\n";
    debug_stream << "ID:  " << get<field::ID>(*records.begin()) << '\n'; // prints first ID in batch
}
//![read_in_batches]
}

{
//![quality_filter]
sequence_file_input fin{std::filesystem::temp_directory_path()/"my.fastq"};

// std::views::filter takes a function object (a lambda in this case) as input that returns a boolean
auto minimum_quality_filter = std::views::filter([] (auto const & rec)
{
    auto qual = get<field::QUAL>(rec) | std::views::transform([] (auto q) { return q.to_phred(); });
    double sum = ranges::accumulate(qual.begin(), qual.end(), 0);
    return sum / std::ranges::size(qual) >= 40; // minimum average quality >= 40
});

for (auto & rec : fin | minimum_quality_filter)
{
    debug_stream << "ID: " << get<field::ID>(rec) << '\n';
}
//![quality_filter]
}

{
//![piping_in_out]
auto tmp_dir = std::filesystem::temp_directory_path();

sequence_file_input fin{tmp_dir/"my.fastq"};
sequence_file_output fout{tmp_dir/"output.fastq"};

// the following are equivalent:
fin | fout;

fout = fin;

sequence_file_output{tmp_dir/"output.fastq"} = sequence_file_input{tmp_dir/"my.fastq"};
//![piping_in_out]
}

{
//![file_conversion]
auto tmp_dir = std::filesystem::temp_directory_path();

sequence_file_output{tmp_dir/"output.fasta"} = sequence_file_input{tmp_dir/"my.fastq"};
//![file_conversion]
}

std::filesystem::remove(std::filesystem::temp_directory_path()/"output.fasta");
std::filesystem::remove(std::filesystem::temp_directory_path()/"output.fastq");
}
