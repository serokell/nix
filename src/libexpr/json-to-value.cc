#include "json-to-value.hh"
#include <nlohmann/json.hpp>
#include "simdjson.cpp"
#include <chrono>
#include <ratio>

namespace nix {

void parse_json(EvalState & state, document::parser::Iterator &pjh, Value & v) {
    switch(pjh.get_type()) { // values: {["slutfnd
    case '{': {
        ValueMap attrs = ValueMap();
        if (pjh.down()) {
            do {
                // pjh is now string
                Value& v2 = *state.allocValue();
                attrs[state.symbols.create(std::move(std::string(pjh.get_string(), pjh.get_string_length())))] = &v2;
                pjh.move_to_value();
                parse_json(state, pjh, v2);
            } while (pjh.next());
            pjh.up();
        }
        state.mkAttrs(v, attrs.size());
        for (auto & i : attrs)
            v.attrs->push_back(std::move(Attr(i.first, i.second)));
    }; break;
    case '[': {
        ValueVector values = ValueVector();
        if (pjh.down()) {
            do {
                Value& v2 = *state.allocValue();
                values.push_back(&v2);
                parse_json(state, pjh, v2);
            } while (pjh.next());
            pjh.up();
        }
        state.mkList(v, values.size());
        for (size_t n = 0; n < values.size(); ++n) {
            v.listElems()[n] = values[n];
        }
    }; break;
    case '"': {
        // todo: handle null byte
        mkStringNoCopy(v, pjh.get_string());
    }; break;
    case 's': case 'l': case 'u':
        mkInt(v, pjh.get_integer()); break;
    case 't': case 'f': mkBool(v, pjh.is_true()); break;
    case 'n': mkNull(v); break;
    case 'd': mkFloat(v, pjh.get_double()); break;
    }
}

void parseJSON(EvalState & state, const string & s_, Value & v)
{
    using namespace std::chrono;
    //high_resolution_clock::time_point t1 = high_resolution_clock::now();
    document::parser parser;
    // todo: check success
    parser.allocate_capacity(s_.length());
    // ugly hack: GC buffer the string_buf so we only allocate once
    // todo: avoid other allocation in allocate_capacity
    parser.doc.string_buf.reset((unsigned char*)GC_malloc_atomic(ROUNDUP_N(5 * s_.length() / 3 + 32, 64)));
    auto [doc, error] = parser.parse(s_);
    if (error) {
        throw JSONParseError(error_message(error));
    }
    auto iterator = document::iterator(doc);
    //high_resolution_clock::time_point t2 = high_resolution_clock::now();
    SymbolCache symbol_cache;
    parse_json(symbol_cache, state, iterator, v);
    // todo: realloc to current_string_buf_loc - string_buf bytes
    parser.doc.string_buf.release();
    //high_resolution_clock::time_point t3 = high_resolution_clock::now();
    //duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    //duration<double> time_span2 = duration_cast<duration<double>>(t3 - t2);
    //std::cerr << "Parse: " << time_span.count() << " seconds." << std::endl;
    //std::cerr << "Process: " << time_span2.count() << " seconds." << std::endl;
}

}
