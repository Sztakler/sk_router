#include <cstdint>
#include <iostream>
#include <vector>
#include <string>

int main()
{
  std::vector<std::vector<std::string>> lines;
  std::vector<std::string> line(3);
  int n;

  std::cin >> n;

  for (int i = 0; i < n; i++)
    {
      std::cin >> line[0];
      std::cin >> line[1];
      std::cin >> line[2];

      lines.push_back(line);
    }


  for (auto line : lines)
    {
      
       for (auto word : line)
      {

       {
        std::cout << word << " ";
       }

      }

  std::cout << "\n";
    }
  return 0;
}
