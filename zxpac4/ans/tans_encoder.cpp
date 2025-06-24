/**
 *
 *
 *
 *
 *
 */

#include "tans_encoder.h"
    

template<class T>
tans_encoder<T>::tans_encoder(int m, const T* Ls, int Ls_len, debug_t debug) : ans_base(m,debug) {
    int sum = 0;

    y_ = NULL;
    k_ = NULL;
    n_ = NULL;
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
    if (y_) { delete[] y_; }
    if (n_) { delete[] n_; }
    if (k_) { delete[] k_; }
}


template<class T>
bool tans_encoder<T>::scaleSymbolFreqs(void) {
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
void tans_encoder<T>::buildEncodingTables(void) {
    y_ = new (std::nothrow) T[M_];
    k_ = new (std::nothrow) T[M_ * Ls_len_];
    n_ = new (std::nothrow) T[M_ * Ls_len_];     // next table

    if (!(y_ && k_ && n_)) {
        std::stringstream ss;
        ss  << __FILE__ << ":" << __LINE__
            << " -> tans_encoder::buildEncodingTables() failed." << std::endl;
        throw std::bad_alloc(ss);
    }




}




