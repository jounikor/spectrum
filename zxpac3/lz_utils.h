#ifndef _LZ_UTILS_H_INCLUDED
#define _LZ_UTILS_H_INCLUDED

#include <exception>

#define EXCEPTION(exp,str) throw(exp(std::string(__FILE__) + ":" +  \
            std::to_string(__LINE__) + " " + #str))

// Encoding specific.. extend as needed, you won't run of numbers ;)
enum class lz_tag_type : int {
    none,               // No match tag assigned.
    // bffp
    bffp_lit,           // tag 0LLLLLLL
    bffp_pmr0,          // tag 10ppnnnn + opt [8]
    bffp_pmr1,          // tag 10ppnnnn + opt [8]
    bffp_pmr2,          // tag 10ppnnnn + opt [8]
    bffp_pmr3,          // tag 10ppnnnn + opt [8]
    bffp_mml2_off16_ctx,// tag 1100nnnn + opt [8] + off [8]
    bffp_mml2_off8,     // tag 1101nnnn + opt [8] + off [8]
    bffp_mml3_off16,    // tag 1110nnnn + opt [8] + off [8] + off [8]
    bffp_mml4_off24     // tag 1111nnnn + opt [8] + off [8] + off [8] + off [8]

    // next ?
};


/**
 * @brief A structure to hold match-length pairs for a LZ string matching engine.
 * @struct match z_alg.h
 */

struct match {
    int32_t pos;                ///< Relative position of a match to the prefix string
    int32_t len;                ///< Length of a match
};

struct match_head {
    match *matches;             ///< An array of found matches.
    int32_t num_matches;        ///< Number of found matches in the \p matches.
    union {
        int32_t literal;        ///< Literal at this location.
        int32_t index_of_max;   ///< Index of the longest match.
    } found;
    // for enumerating the best match in this forward window
    lz_tag_type tag;            ///< Type of tag selected for this position.
    int selected_match;         ///< An index into the \p macthes array.
    int adjusted_length;        ///< If nneded length < matches[selected_match].len.
};

#define max_match_index found.index_of_max

class matches {
    match_head *m_all_matches;  ///< m_max_forward of forward match heads.
    match *m_pos_matches;       ///< m_max_forward*m_max_matches of individul matches.
    int m_max_chain;            ///< Max number of matches in a single position.
    uint32_t m_max_forward;     ///< Number of forward matches.
    uint32_t m_max_forward_minus_1;
public:
    matches(int size, int num);
    virtual ~matches(void);
    match_head& operator[](int);
    match_head& at(int);
    const match_head& operator[](int) const;
    const match_head& at(int) const;

    uint32_t get_max_forward(void) const { return m_max_forward; }
    int get_max_chain(void) const { return m_max_chain; }
    uint32_t get_max_forward_mask(void) const { return m_max_forward_minus_1; }
};


#endif  // _LS_UTILS_H_INCLUDED
