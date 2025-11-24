/**
 * @file tans_encoder.h
 * @author Jouni 'Mr.Spiv' korhonen
 * @version 0.1
 * @copyright The Unlicense
 *
 *
 *
 *
 */

#ifndef _TANS_ENCODER_H_INCLUDED
#define _TANS_ENCODER_H_INCLUDED

#include <cstdint>
#include "ans.h"

/*
 *
 *
 */
template<class T, int M>
class tans_encoder : public ans_base {
    int* Ls_;
    T (*next_state_)[M];
    T (*k_)[M];
    T* symbol_last_;
    int Ls_len_;
    T symbol_to_k_[M];
    ans_state_t state_;

    bool scaleSymbolFreqs(void);
    void buildEncodingTables(void);
    void free_tables(void);

public:
    tans_encoder(void);
    tans_encoder(const int* Ls, int n);
    ~tans_encoder();

    void init_tans(const int* Ls, int n);

    const int* get_scaled_Ls(void) const;
    int get_Ls_len(void) const;
    ans_state_t init_encoder(T&);
    ans_state_t done_encoder(void) const;
    ans_state_t encode(T s, uint8_t& k, uint32_t& b);
    ans_state_t get_state(void) {
        return state_;
    };
    void dump(void);
};

template<class T, int M>
tans_encoder<T,M>::tans_encoder(void) : ans_base(M)
{
    k_ = NULL;
    Ls_ = NULL;
    next_state_ = NULL;
    symbol_last_ = NULL;
    Ls_len_ = 0;
}

template<class T, int M>
tans_encoder<T,M>::tans_encoder(const int* Ls, int Ls_len) : ans_base(M)
{
    k_ = NULL;
    Ls_ = NULL;
    next_state_ = NULL;
    symbol_last_ = NULL;
    init_tans(Ls,Ls_len);
}


template<class T, int M>
void tans_encoder<T,M>::init_tans(const int* Ls, int Ls_len)
{
    free_tables();
    Ls_len_ = Ls_len;

    // Yes.. we make a copy of the original array..
    Ls_ = new (std::nothrow) int[Ls_len]; 
    if (Ls_ == NULL) {
        throw std::bad_alloc();
    }

    for (int i = 0; i < Ls_len; i++) {
        Ls_[i] = Ls[i];
    }

    // Prepare symbols frequencies and build tables
    scaleSymbolFreqs();
    buildEncodingTables();
}

template<class T, int M>
tans_encoder<T,M>::~tans_encoder()
{
    free_tables();
}

template<class T, int M>
void tans_encoder<T,M>::free_tables(void)
{
    if (Ls_) { delete[] Ls_; Ls_ = NULL; }
    if (symbol_last_) { delete[] symbol_last_; symbol_last_ = NULL; }
    if (next_state_) { delete[] next_state_; next_state_ = NULL; }
    if (k_) { delete[] k_; k_ = NULL; }
}


template<class T, int M>
bool tans_encoder<T,M>::scaleSymbolFreqs(void)
{
    int L = 0;

    for (int i = 0; i < Ls_len_; i++) {
        L += Ls_[i];
    }
    
    if (L != M) {
        int i;
        // Total sum L is not a power of two.. scale up or down.
        // https://stackoverflow.com/questions/31121591/normalizing-integers
        // The last reminder gets always added to the last item in the list..
        
        int reminder = 0;
        int underflow = 0;

        for (i=0; i < Ls_len_; i++) {
            if (Ls_[i] == 0) {
                continue;
            }
        
            int count = Ls_[i] * M + reminder;
            int tmp = count / L;

            // Check if we have an underflow..
            if (tmp == 0) {
                ++underflow;
                tmp = 1;
            }

            Ls_[i] = tmp;
            reminder = count % L;
        }
    
        // Handle underflows
        i = Ls_len_ - 1;
        while (underflow > 0 && i > 0) {
            if (Ls_[i] > 1) {
                Ls_[i] -= 1;
                --underflow;
            }
          --i;
        }

        for (i = L = 0; i < Ls_len_; i++) {
            L += Ls_[i];
        }
    }

    if (L != M) {
        return false;
    }

    return true;
}


template<class T, int M>
void tans_encoder<T,M>::buildEncodingTables(void)
{
    symbol_last_ = new (std::nothrow) T[Ls_len_];
    next_state_ = reinterpret_cast<T(*)[M]>(new (std::nothrow) T[M * Ls_len_]);
    k_ = reinterpret_cast<T(*)[M]>(new (std::nothrow) T[M * Ls_len_]);

    T (*k)[M] = reinterpret_cast<T(*)[M]>(k_);
    T (*next_state)[M] = reinterpret_cast<T(*)[M]>(next_state_);

    if (!(symbol_last_ && k_ && next_state_)) {
        throw std::bad_alloc();
    }

    // The initial state to start with..
    int xp = INITIAL_STATE_;
    int last_xp = INITIAL_STATE_;

    for (int s = 0; s < Ls_len_; s++) {
        int c = Ls_[s];

        for (int p = c; p < 2*c; p++) {
            // Note that we make the table indices reside between [0..L),
            // since values within [L..2L) % M is within [0..M). This is 
            // a simple algorithmic optimization to avoid extra index
            // adjusting during decoding..
            
            // Get the k's for the given symbol..
            uint8_t k_tmp = get_k_(p,M);
            int yp_tmp = p << k_tmp;

            for (int yp_pos = yp_tmp; yp_pos < yp_tmp + (1 << k_tmp); yp_pos++) {
                // next table for each symbol.. this array will explode in size when the
                // symbol set gets bigger.
                next_state[s][yp_pos & (M-1)] = xp;
                
                // k table for each symbol.. this array will explode in size when the
                // symbol set gets bigger.
                k[s][yp_pos & (M-1)] = k_tmp;
            }

            // advance to the next state..
            last_xp = xp;
            xp = spreadFunc_(xp);
        }

        // Record the final state for the symbol. One of these will be used for the
        // initial state when starting encoding.
        symbol_last_[s] = last_xp; 
    }
}

template<class T, int M>
const int* tans_encoder<T,M>::get_scaled_Ls(void) const
{
    return Ls_;
}

template<class T, int M>
int tans_encoder<T,M>::get_Ls_len(void) const
{
    return Ls_len_;
}

template<class T, int M>
ans_state_t tans_encoder<T,M>::init_encoder(T& s) {
    state_ = symbol_last_[s];
    return state_;
}

template<class T, int M>
ans_state_t tans_encoder<T,M>::done_encoder(void) const
{
    // Dummy function.. subject to removal.
    return state_;
}

template<class T, int M>
ans_state_t tans_encoder<T,M>::encode(T s, uint8_t& k, uint32_t& b)
{
    k = k_[s][state_];
    b = state_ & ((1 << k) - 1);
    state_ = next_state_[s][state_];
    return state_;
}

template<class T, int M>
void tans_encoder<T,M>::dump(void)
{
    std::cerr << "-- Scaled Ls --\n";
    const int* ls = get_scaled_Ls();
    int n;
    
    std::cerr << std::dec << std::left;
    
    for (n = 0; n < get_Ls_len(); n++) {
        std::cerr << std::setw(8) << n;
    }
    std::cerr << "\n";

    for (n = 0; n < get_Ls_len(); n++) {
        std::cerr << std::setw(8) << ls[n];
    }
    std::cerr << std::right << "\n";


    //std::cerr << "-- k table: ------------------------------------------------\n";
    //std::cerr << "-- b table: ------------------------------------------------\n";
    //std::cerr << "-- next table: ---------------------------------------------\n";
    //std::cerr << "-- Ls table: -----------------------------------------------\n";
    //std::cerr << "-- last symbol table: --------------------------------------\n";
}


#endif      // _TANS_ENCODER_H_INCLUDED
