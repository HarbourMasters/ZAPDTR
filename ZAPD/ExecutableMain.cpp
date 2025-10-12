extern "C" int zapd_main(int argc, char* argv[], size_t* extractCount, size_t* totalExtract);

int main(int argc, char* argv[], size_t* extractCount = nullptr, size_t* totalExtract = nullptr) {
    return zapd_main(argc, argv, extractCount, totalExtract);
}
