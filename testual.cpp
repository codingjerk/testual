// TODO: let tests generate uniform json data and use dfferent processors to format and show output result
// TODO: coverage (just add examples to readme)
// TODO: init (or setup) code blocks to contain common tests data
// TODO: extract utils, parse and generation functions to separate files
// TODO: remove this TODOs and move it to issue tracker
// TODO: allow to fail tests with timeout
// TODO: write testual_run script to collect all tests

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>

typedef std::vector<std::string> words_t;
typedef std::map<std::string, std::string> name_table_t;
typedef std::map<std::string, name_table_t> file_table_t;

struct test_head_t {
    std::string test_name;
    std::string function_name;

    bool valid;
};

words_t split_by_words(const std::string& s) {
    words_t result;

    std::stringstream ss(s);
    while (!ss.eof()) {
        std::string item; ss >> item;
        result.push_back(item);
    }

    return result;
}

std::string extract_test_name(words_t& words) {
    int last_id = words.size() - 2;

    std::stringstream ss;
    for (int i = 1; i < last_id; ++i) {
        ss << words[i] << " ";
    }

    ss << words[last_id];

    return ss.str();
}

std::string replace_bad_characters(const std::string& str) {
    std::string result;

    for (auto c: str) {
        if (isalnum(c) || c == '_') {
            result += c;
        // @TODO: use char -> sign name map
        } else if (c == '!') {
            result += "__exclamation_mark__";
        } else if (c == '=') {
            result += "__equals_sign__";
        } else if (c == '/') {
            result += "__slash__";
        } else if (c == '.') {
            result += "__dot__";
        } else {
            std::cerr << "Unknown character in test name: " << c << std::endl;
            exit(1);
        }
    }

    return result;
}

std::string extract_function_name(words_t& words) {
    int last_id = words.size() - 2;

    std::stringstream ss; ss << "__test__";
    for (int i = 1; i < last_id; ++i) {
        ss << words[i] << "_";
    }

    ss << words[last_id];

    return replace_bad_characters(ss.str());
}

test_head_t try_get_test_head(const std::string& line) {
    test_head_t invalid_head; invalid_head.valid = false;
    words_t words = split_by_words(line);

    if (words.size() < 3) return invalid_head;
    if (words[0] != "test") return invalid_head;
    if (words.back() != "{") return invalid_head;

    test_head_t result;
    result.valid = true;

    result.test_name = extract_test_name(words);
    result.function_name = extract_function_name(words);

    return result;
}

// HACK: used to allow user define own non-test functions
bool test_started = false;

void parse_test_start(const std::string& file_prefix, std::ostream& out, const test_head_t& test_head, name_table_t& name_table) {
    test_started = true;

    out << "// test " << test_head.test_name << std::endl;
    out << "struct testual_error_t " << file_prefix + test_head.function_name << "() {" << std::endl;

    name_table.emplace(test_head.test_name, file_prefix + test_head.function_name);
}

void parse_test_end(std::ostream& out) {
    test_started = false;

    out << "    struct testual_error_t err; err.is_error = 0;" << std::endl;
    out << "    return err;" << std::endl;
    out << "}" << std::endl;
}

void parse_line(const std::string& file_prefix, std::ostream& out, const std::string& line, name_table_t& name_table) {
    test_head_t test_head = try_get_test_head(line);

    if (test_head.valid) {
        parse_test_start(file_prefix, out, test_head, name_table);
    } else if (line[0] == '}' && test_started) {
        parse_test_end(out);
    } else { // just put original line
        out << line << std::endl;
    }
}

// TODO: write real parser instead of parse line-by-line
name_table_t parse_file(std::string file_name, std::istream& in, std::ostream& out) {
    std::string file_prefix = "___" + replace_bad_characters(file_name);

    name_table_t name_table;
    for (std::string line; std::getline(in, line); ) {
        parse_line(file_prefix, out, line, name_table);
    }

    return name_table;
}

void generate_header(std::ostream& out) {
    out << "#include <testual.h>" << std::endl;
    out << "#include <stdio.h>" << std::endl;
    out << "#include <time.h>" << std::endl;

    out << std::endl;
}

const char lvl1[] = "  ";
const char lvl2[] = "    ";
const char lvl3[] = "      ";

const char color_default[] = "\\033[0m";

const char color_red[] = "\\033[0;31m";
const char color_green[] = "\\033[0;32m";
const char color_yellow[] = "\\033[0;33m";
const char color_white[] = "\\033[0;37m";

const char color_red_bold[] = "\\033[1;31m";
const char color_green_bold[] = "\\033[1;32m";
const char color_white_bold[] = "\\033[1;37m";

std::string colorize(const std::string& str, const std::string& color) {
    return color + str + color_default;
}

void generate_main_header(std::ostream& out) {
    out << "int main() {" << std::endl;
    out << lvl1 << "struct testual_error_t err;" << std::endl;
    out << lvl1 << "int total = 0, passed = 0, failed = 0;" << std::endl;

    out << std::endl;
}

void generate_start_time_measue(std::ostream& out, const std::string& variable) {
    out << lvl1 << "clock_t " << variable << " = clock();" << std::endl;
}

std::string generate_unique_postfix() {
    static int n = 0;

    std::stringstream ss; ss << n++;

    return ss.str();
}

void generate_end_time_measure(std::ostream& out, const std::string& start_variable, const std::string& ms_variable) {
    std::string tmp_var = "tmp_var_" + generate_unique_postfix();

    out << lvl1 << "clock_t " << tmp_var << " = clock();" << std::endl;
    out << lvl1 << "int "<< ms_variable << " = (double)(" << tmp_var << " - " << start_variable << ") * 1000 / CLOCKS_PER_SEC;" << std::endl;
}

void generate_printf(std::ostream& out, const std::string& lvl, const std::string& text, const std::string& postfix="") {
    out << lvl << "printf(\"" << text << "\"" << postfix << ");" << std::endl;
}

void generate_printf_line(std::ostream& out, const std::string& lvl, const std::string& text, const std::string& postfix="") {
    generate_printf(out, lvl, text + "\\n", postfix);
}

void generate_printf_line_params(std::ostream& out, const std::string& lvl, const std::string& text, const std::string& params) {
    generate_printf(out, lvl, text + "\\n", ", " + params);
}

void generate_show_file_name(std::ostream& out, const std::string& file_name) {
    generate_printf_line(out, lvl1, colorize(file_name, color_white_bold));
}

void generate_show_time(std::ostream& out, const std::string& lvl, const std::string& time_var) {
    std::string green_time  = colorize(" (%dms)", color_green);
    std::string yellow_time = colorize(" (%dms)", color_yellow);
    std::string red_time    = colorize(" (%dms)", color_red);

    out << lvl << "if (" << time_var << " <= 10) {" << std::endl;
    generate_printf_line_params(out, lvl + lvl1, green_time, time_var);
    out << lvl << "} else if (" << time_var << " <= 100) {" << std::endl;
    generate_printf_line_params(out, lvl + lvl1, yellow_time, time_var);
    out << lvl << "} else {" << std::endl;
    generate_printf_line_params(out, lvl + lvl1, red_time, time_var);
    out << lvl << "}" << std::endl;
}

void generate_test_passed(std::ostream& out, const std::string& lvl, const std::string& test_name, const std::string& time_var) {
    std::string icon = colorize("✔", color_green_bold);
    std::string title = colorize(test_name, color_white);
    generate_printf(out, lvl, lvl1 + icon + " " + title);
    generate_show_time(out, lvl, time_var);

    out << lvl << "passed++;" << std::endl;
}

void generate_test_failed(std::ostream& out, const std::string& lvl, const std::string& test_name, const std::string& time_var) {
    std::string icon = colorize("✘", color_red_bold);
    std::string title = colorize(test_name, color_red_bold);
    generate_printf(out, lvl, lvl1 + icon + " " + title);

    generate_printf_line_params(out, lvl, lvl2 + colorize("%s", color_red_bold), "err.error_description");

    out << lvl << "failed++;" << std::endl;
}

void generate_check_test_result(std::ostream& out, const std::string& lvl, const std::string& test_name, const std::string& time_var) {
    out << lvl << "if (err.is_error) {" << std::endl;
    generate_test_failed(out, lvl + lvl1, test_name, time_var);
    out << lvl << "} else {" << std::endl;
    generate_test_passed(out, lvl + lvl1, test_name, time_var);
    out << lvl << "}" << std::endl;
}

void generate_test_run(std::ostream& out, const std::string& test_name, const std::string& function_name) {
    out << lvl1 << "total++;" << std::endl;

    generate_start_time_measue(out, "__start__" + function_name);
    out << lvl1 << "err = " << function_name << "();" << std::endl;
    generate_end_time_measure(out, "__start__" + function_name, "__time__" + function_name);

    generate_check_test_result(out, lvl1, test_name, "__time__" + function_name);
    out << std::endl;
}

void generate_file_testing(std::ostream& out, const std::string& file_name, const name_table_t& name_table) {
    generate_show_file_name(out, file_name);

    for (auto n: name_table) {
        std::string test_name = n.first;
        std::string function_name = n.second;

        generate_test_run(out, test_name, function_name);
    }
}

void generate_show_summary_passed(std::ostream& out, const std::string& lvl) {
    std::string summary = colorize("%d/%d passed", color_green_bold) + " " +
        colorize("(%d total asserts, %dms spent)", color_white);

    generate_printf_line_params(out, lvl, summary, "passed, total, __testual__assert_count, total_spent");
}

void generate_show_summary_failed(std::ostream& out, const std::string& lvl) {
    std::string summary = colorize("%d failed!", color_red_bold) + " " +
        colorize("%d/%d passed", color_white) + " " +
        colorize("(%d total asserts, %dms spent)", color_white);

    generate_printf_line_params(out, lvl, summary, "failed, passed, total, __testual__assert_count, total_spent");
}

void generate_show_summary(std::ostream& out) {
    generate_printf_line(out, "", "");

    out << lvl1 << "if (failed == 0) {" << std::endl;
    generate_show_summary_passed(out, lvl2);
    out << lvl1 << "} else {" << std::endl;
    generate_show_summary_failed(out, lvl2);
    out << lvl1 << "}" << std::endl;
}

void generate_main(std::ostream& out, file_table_t &file_table) {
    generate_main_header(out);

    generate_start_time_measue(out, "begin");
    for (auto file: file_table) {
        std::string file_name = file.first;
        name_table_t name_table = file.second;

        generate_file_testing(out, file_name, name_table);
    }
    generate_end_time_measure(out, "begin", "total_spent");

    generate_show_summary(out);

    out << lvl1 << "return failed;" << std::endl;

    out << "}" << std::endl;
}

file_table_t generate_content(std::ostream& out, int argc, const char **argv) {
    file_table_t file_table;
    if (argc == 1) {
        name_table_t name_table = parse_file("tests", std::cin, out);
        file_table.emplace("tests", name_table);

        return file_table;
    }

    for (int i = 1; i < argc; ++i) {
        std::ifstream file; file.open(argv[i]);
        name_table_t name_table = parse_file(argv[i], file, out);
        file_table.emplace(argv[i], name_table);
    }

    out << std::endl;

    return file_table;
}

int main(int argc, const char **argv) {
    generate_header(std::cout);
    file_table_t file_table = generate_content(std::cout, argc, argv);
    generate_main(std::cout, file_table);

    return 0;
}
