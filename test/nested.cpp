#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>

int main(int argc, char **argv)
{
   if (argc == 2) {
      std::map<std::string, std::vector<size_t>> positions;

      std::ifstream fin(argv[1]);
      std::string word;
      while (fin >> word)
         positions[word].push_back((size_t)fin.tellg() - word.length());
      fin.close();

      for (auto &pair : positions) {
         std::cout << pair.first;
         for (auto &pos : pair.second)
            std::cout << " " << pos;
         std::cout << "\n";
      }
   }
}
