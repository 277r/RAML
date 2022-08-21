// RS's Adaptive Movie Language
/* 
    the reason why i wrote this is because movies often have one subject and a lot of words that occur often,
    this format is made to need less bytes for often occuring words and more bytes for less common words, 
    but even then the max size a word can take up is (length of the word) + log256(length of word) + log256(amount of words in the script)
    so the max overhead for the word "quintillion" for a movie with 65537 words is 11 + 1 + 3 
    but the overhead might be saved by the compression of the other words

*/

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>

// use pragma pack 1 to make structs not include padding for faster execution
#pragma pack(1)

struct raml_header {
    // start code for ID'ing, can be removed if needed by application
    // because of endian encoding this will show up as the reverse on disk on x86 systems
    unsigned int code = 'LMAR';
    // max word length
    // the actual max word length equals 2^8 * 2^(mwl - 1) - 1
    // so if all words are less than 256 long, it will just be 2^8 * 2^(1 - 1) - 1 = 2^8 
    unsigned char wml;

    // amount of words the file contains
    unsigned long long wc;

};

struct word {
    // the max length this can contain is the mwl rounded up to the largest byte size, so if all words are less than 256 bytes long, it will be only one byte in length
    unsigned long long length;
    // the word itself 
    unsigned char data[];
};

// the stream in the file of words is just a list of indexes, the more often a word occurs, the smaller the index
struct word_index {
    /*
        bits:
        0b0xxxxxxx:
            there will be another word index after this one, take the 7 bits from that because the word index is larger than 127
        0b1xxxxxxx
            this is the last word index, after this a whole new word starts
        
        the 7 'x' bits contain the value of the word index

    */
    unsigned char index;
};



// the structs defined after this are structs used for the program to run, not the structs with bit layout on the disk

struct wv_block {
    std::string word;
    int occurence;
    // can be ignored until later 
    int position;
};


// helper function so std::sort can sort structs based on values inside
bool wv_sort(wv_block l, wv_block r){
    // sort from high to low
    return l.occurence > r.occurence;
}

std::string wml_to_string(int pos, int wml){
    std::string data;
    for (int i = 0; i <= wml; i++){
        data += (unsigned char)(pos % 256);
        pos /= 256; 
    }
    // reverse string
    std::string rev;
    for (int i = data.size() - 1; i >= 0; i--){
        rev += data[i];
    }

    return rev;

}

std::string wordpos_to_string(int pos){
    std::string d;
    
    if (pos == 0){
        d += (unsigned char)128;
    }

    while (pos > 0){
        // get 7 bit part
        int t = pos & 127;
        // if the position is smaller than 127 bits, append the first bit to set end of bit count
        if (pos < 128){
            t |= 128;
        }
        d += (unsigned char) t;
        pos /= 128;
    }
    return d;
}

int find(std::string word, std::vector<wv_block> wordlist){
    for (int i = 0; i < wordlist.size(); i++){
        if (wordlist[i].word == word){
            return i;
        }
    }
    return -1;
}


void encode(std::string data, char *outfile){
    std::vector<wv_block> words;
    raml_header x;
    // set word count to zero
    x.wc = 0;

    // loop through all words and fill vector with list of words
    std::string tmp_word;
    for (int i = 0; i < data.size(); i++){
        
        tmp_word.clear();
        // get all nonspace characters into tmp_word
        while (data[i] != ' ' && i < data.size()){
            tmp_word += data[i];
            i++;
        }
        // add space to end of tmp_word
        if (i < data.size()){
            tmp_word += data[i];
        }
        
        // word created, check if it already exists
        bool word_exists = false;
        for (int ii = 0; ii < words.size(); ii++){
            if (words[ii].word == tmp_word){
                word_exists = true;
                words[ii].occurence += 1;
                // increment word count
                x.wc++;
                break;
            }
        }
        // if word doesn't exist, append the word to the list with occurence of 1 and position of 0 (doesn't matter)
        if (word_exists == false){
            words.push_back({tmp_word, 1, 0});
            // increment word count
            x.wc++;
        }
    }
    

    // sort based on occurence
    std::sort(words.begin(), words.end(), wv_sort);
    // replace all words with indexes in list




    // calculate longest word length for wml variable (important)
    int max_word_length = 0;
    for (int i = 0; i < words.size(); i++){
        if (words[i].word.size() >= max_word_length){
            
            max_word_length = words[i].word.size();
        }
    }
    // calculate wml as number that can be reversed in the equation
    x.wml = log2(max_word_length) / log2(256);
    // debug
    std::cout << "max word length: " << (int)x.wml << "\n";

    // output header, list of words (with mwl), output list of indexes
    std::ofstream o(outfile, std::ios::binary);
    // write header
    o.write((const char*)&x, sizeof(raml_header));
    // write words 
    for (int i = 0; i < words.size(); i++){
        // output length of word
        // turn wordlength into wml
        o << wml_to_string(words[i].word.length(), x.wml);
        // output word itself
        o << words[i].word;

    }

    // loop through input and write words
    for (int i = 0; i < data.size(); i++){
        
        tmp_word.clear();
        // get all nonspace characters into tmp_word
        while (data[i] != ' ' && i < data.size()){
            tmp_word += data[i];
            i++;
        }
        // add space to end of tmp_word
        if (i < data.size()){
            tmp_word += data[i];
        }
        
        // tmp word is created, look it up in word list
        int word_pos = find(tmp_word, words);
        
        // turn the word position into 7 bit blocks
        o << wordpos_to_string(word_pos);

    }

    o.close();


}



std::string decode(char *infile){

}






int main(int argc, char *argv[]){
    // too lazy to implement my option system right now, todo for later

    std::ifstream t("in.txt");
    std::stringstream b;
    b << t.rdbuf();

    encode(b.str(), "h.txt" );
}