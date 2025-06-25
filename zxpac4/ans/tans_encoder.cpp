/**
 * @file src/tans_encoder.cpp
 * @version 0.1
 * @author Jouni 'Mr.Spiv' korhonen
 * @copyright The Unlicense
 * @brief This file implement the tANS encoder class template.
 */

#include "tans_encoder.h"
    

template<class T>
tans_encoder<T>::tans_encoder(int m, const T* Ls, int Ls_len, debug_t debug) : ans_base(m,debug) {
    int sum = 0;

    next_state_ = NULL;
    symbol_last_ = NULL;
    k_ = NULL;
    Ls_ = NULL;
    Ls_len_ = Ls_len;

    // Yes.. we make a copy of the original array..
    Ls_ = new (std::nothrow) T[Ls_len]; 
    if (Ls_ == NULL) {
        std::stringstream ss;
        ss << __FILE__ << ":" << __LINE__ << " -> tans_encoder() constructor failed." << std::endl;
        throw std::bad_alloc(ss);
    }

    for (int i = 0; i < Ls_len; i++) {
        Ls_[i] = Ls[i];
    }

    // Prepare symbols frequencies and build tables
    scaleSymbolFreqs();
    buildEncodingTables();
}

template<class T>
tans_encoder<T>::~tans_encoder(void)
{
    if (Ls_) { delete[] Ls_; }
    if (symbol_last_) { delete[] symbol_last_; }
    if (next_state_) { delete[] next_state_; }
    if (k_) { delete[] k_; }
}


template<class T>
bool tans_encoder<T>::scaleSymbolFreqs(void)
{
    int L = 0;

    for (int i = 0; i < Ls_len_; i++) {
        L += Ls_[i];
    }
    
    if (L != M_) {
        int i;
        // Total sum L is not a power of two.. scale up or down.
        // https://stackoverflow.com/questions/31121591/normalizing-integers
        // The last reminder gets always added to the last item in the list..
        
        TRACE_DBUG("L should be: " << M_ << ", L is now: " << L << std::endl)
        
        int reminder = 0;
        int underflow = 0;

        for (i=0; i < Ls_len_; i++) {
            if (Ls_[i] == 0) {
                continue;
            }
        
            int count = Ls_[i] * M_ + reminder;
            int tmp = count / L;

            // Check if we have an underflow..
            if (tmp == 0) {
                ++underflow;
                tmp = 1;
                TRACE_DBUG("Ls[" << i << "] would become 0" << std::endl)
            }

            Ls_[i] = tmp;
            reminder = count % L;
        }
    
        TRACE_DBUG("Number of underfloes is " << underflow << std::end)

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

        TRACE_DBUG("New Ls = [")
        for (i = 0; i < Ls_len_; i++) {
            TRACE_DBUG(Ls_[i] << " ")
        }
        TRACE_DBUG("]" << std::endl)
    }

    if (L != M_) {
        TRACE_WARN("M (" << M_ << ") != L (" << L << ")" << std::endl)
        return false;
    }

    return true;
}


template<class T>
void tans_encoder<T>::buildEncodingTables(void)
{
    symbol_last_ = new (std::nothrow) T[Ls_len_];
    next_state_ = new (std::nothrow) T[M_ * Ls_len_];     // next table
    k_ = new (std::nothrow) T[M_ * Ls_len_];

    T(*k)[Ls_len_] = k_;
    T(*next_state)[Ls_len_] = next_state_;

    if (!(symbol_last_ && k_ && next_state_)) {
        std::stringstream ss;
        ss  << __FILE__ << ":" << __LINE__
            << " -> tans_encoder::buildEncodingTables() failed." << std::endl;
        throw std::bad_alloc(ss);
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
            int k_tmp = get_k_(p,M_);
            int yp_tmp = p << k_tmp;

            for (int yp_pos = yp_tmp; yp_pos < yp_tmp + (1 << k_tmp); yp_pos++) {
                TRACE_DBUG("s: " << s << ", p: " << p ", xp: " << xp    \
                    << ", k_tmp: " << k_tmp << ", yp_tmp: " << yp_tmp   \
                    << ", yp_pos: " << yp_pos << std::endl)
            
                // next table for each symbol.. this array will explode in size when the
                // symbol set gets bigger.
                next_state[s][yp_pos & M_MASK_] = k_tmp;
                
                // k table for each symbol.. this array will explode in size when the
                // symbol set gets bigger.
                k[s][yp_pos & M_MASK_] = k_tmp;
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




