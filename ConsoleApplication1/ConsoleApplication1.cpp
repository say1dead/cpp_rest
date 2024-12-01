#include <iostream>
#include "pugixml.hpp"
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <curl/curl.h>
#include <Windows.h>
using namespace std;

// Функция обратного вызова для записи данных в файл
size_t write_data(void* ptr, size_t size, size_t nmemb, std::ofstream& stream) {
    stream.write((char*)ptr, size * nmemb);
    return size * nmemb;
}

void getDateParts(const std::string& dateStr, int& day, int& month, int& year) {
    std::istringstream iss(dateStr);
    char dot; // для символа точки в формате даты "дд.мм.гггг"

    // Считываем день, месяц и год из строки
    iss >> day >> dot >> month >> dot >> year;
}

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int month, int year) {
    if (month == 2) {
        return isLeapYear(year) ? 29 : 28;
    }
    else if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    else {
        return 31;
    }
}

std::vector<std::vector<int>> extractDates(const std::string& dateRange) {
    std::vector<std::vector<int>> result;

    std::stringstream ss(dateRange);
    std::string startDate, endDate;
    std::getline(ss, startDate, '-');
    std::getline(ss, endDate);

    int startDay, startMonth, startYear, endDay, endMonth, endYear;
    sscanf_s(startDate.c_str(), "%d.%d.%d", &startDay, &startMonth, &startYear);
    sscanf_s(endDate.c_str(), "%d.%d.%d", &endDay, &endMonth, &endYear);

    for (int day = startDay, month = startMonth, year = startYear; ; ) {
        result.push_back({ day, month, year });

        if (day == endDay && month == endMonth && year == endYear) {
            break;
        }

        day++;
        if (day > daysInMonth(month, year)) {
            day = 1;
            month++;
        }
        if (month > 12) {
            month = 1;
            year++;
        }
    }

    return result;
}

double stringToDouble(const std::string& str) {
    double result = 0.0;
    double fraction = 1.0;
    bool isFraction = false;

    for (char c : str) {
        if (c == ',') {
            isFraction = true;
            continue;
        }

        if (!isFraction) {
            result = result * 10 + (c - '0');
        }
        else {
            fraction /= 10;
            result += (c - '0') * fraction;
        }
    }

    return result;
}

int main(int argc, char* argv[]) {

    string param_vname = argv[1];
    string param_vnom = argv[2];
    string param_date = argv[3];
    string param_datedate = argv[4];
    string param_filename = argv[5];

    if (param_date == "-" && param_datedate == "-") {
        CURL* curl;
        CURLcode res;
        std::ofstream xmlFile;
        const char* url = "http://www.cbr.ru/scripts/XML_daily.asp";

        curl = curl_easy_init(); // инициализация объекта CURL

        if (curl) {
            xmlFile.open("output.xml"); // открываем файл для записи в бинарном режиме

            curl_easy_setopt(curl, CURLOPT_URL, url); // устанавливаем URL для загрузки
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xmlFile); // указываем файл для записи полученных данных
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // устанавливаем функцию обратного вызова для записи данных

            res = curl_easy_perform(curl); // выполняем запрос

            if (res != CURLE_OK) {
                std::cerr << "DFJSMHDBFHSD: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl); // освобождаем ресурсы
            xmlFile.close(); // закрываем файл

            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file("output.xml");

            // Извлекаем дату из узла "ValCurs"
            std::string date = doc.child("ValCurs").attribute("Date").as_string();

            string outputInfo; // Переменная для хранения конечной информации

            for (pugi::xml_node valute : doc.child("ValCurs").children("Valute")) {
                std::string charCode = valute.child_value("CharCode");
                if (charCode == param_vname) {
                    std::string value_str = valute.child_value("Value");
                    std::string nominal = valute.child_value("Nominal");

                    // Проверяем, если параметр vnom не равен "-", то пересчитываем значение value и меняем nominal
                    if (param_vnom != "-") {
                        string nom_for_one = valute.child_value("Value");
                        double nom_for_one_double = stringToDouble(nom_for_one);
                        double param_vnom_double = stringToDouble(param_vnom);
                        double value = nom_for_one_double * param_vnom_double;
                        value_str = to_string(value);
                        nominal = param_vnom;
                    }

                    outputInfo = param_vname + ' ' + nominal + ' ' + date + ' ' + value_str; // Сохраняем значение и номинал в outputInfo

                    break;  // Выход из цикла после найденной валюты
                }
            }

            if (param_filename != "-") {
                // Если param_filename не равно "-", записываем информацию в указанный файл
                std::ofstream outputFile(param_filename);
                if (outputFile.is_open()) {
                    outputFile << outputInfo;
                    outputFile.close();
                }
                else {
                    std::cerr << "Ошибка открытия файла для записи." << std::endl;
                }
            }
            else {
                cout << outputInfo;
            }
        }
    }
    else if (param_date != "-" && param_datedate == "-") {

        CURL* curl;
        CURLcode res;
        std::ofstream xmlFile;
        int d, m, y;
        getDateParts(param_date, d, m, y);
        string base = "http://www.cbr.ru/scripts/XML_daily.asp?date_req=";
        string url = base + (d < 10 ? "0" : "") + to_string(d) + "/" + (m < 10 ? "0" : "") + to_string(m) + "/" + to_string(y);


        curl = curl_easy_init(); // инициализация объекта CURL

        if (curl) {
            xmlFile.open("output.xml"); // открываем файл для записи в бинарном режиме

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // устанавливаем URL для загрузки
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xmlFile); // указываем файл для записи полученных данных
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // устанавливаем функцию обратного вызова для записи данных

            res = curl_easy_perform(curl); // выполняем запрос

            if (res != CURLE_OK) {
                std::cerr << "DFJSMHDBFHSD: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl); // освобождаем ресурсы
            xmlFile.close(); // закрываем файл

            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load_file("output.xml");

            // Извлекаем дату из узла "ValCurs"
            std::string date = doc.child("ValCurs").attribute("Date").as_string();

            string outputInfo; // Переменная для хранения конечной информации

            for (pugi::xml_node valute : doc.child("ValCurs").children("Valute")) {
                std::string charCode = valute.child_value("CharCode");
                if (charCode == param_vname) {
                    std::string value_str = valute.child_value("Value");
                    std::string nominal = valute.child_value("Nominal");

                    // Проверяем, если параметр vnom не равен "-", то пересчитываем значение value и меняем nominal
                    if (param_vnom != "-") {
                        string nom_for_one = valute.child_value("Value");
                        double nom_for_one_double = stringToDouble(nom_for_one);
                        double param_vnom_double = stringToDouble(param_vnom);
                        double value = nom_for_one_double * param_vnom_double;
                        value_str = to_string(value);
                        nominal = param_vnom;
                    }

                    outputInfo = param_vname + ' ' + nominal + ' ' + date + ' ' + value_str; // Сохраняем значение и номинал в outputInfo

                    break;  // Выход из цикла после найденной валюты
                }
            }

            if (param_filename != "-") {
                // Если param_filename не равно "-", записываем информацию в указанный файл
                std::ofstream outputFile(param_filename);
                if (outputFile.is_open()) {
                    outputFile << outputInfo;
                    outputFile.close();
                }
                else {
                    std::cerr << "Ошибка открытия файла для записи." << std::endl;
                }
            }
            else {
                cout << outputInfo;
            }
        }
    }
    else if (param_date == "-" && param_datedate != "-") {
        vector<vector<int>> dates = extractDates(param_datedate);
        vector<string> answers;
        int size = 0;
        for (const auto& date : dates) {
            CURL* curl;
            CURLcode res;
            std::ofstream xmlFile;
            string base = "http://www.cbr.ru/scripts/XML_daily.asp?date_req=";
            string url = base + (date[0] < 10 ? "0" : "") + to_string(date[0]) + "/" + (date[1] < 10 ? "0" : "") + to_string(date[1]) + "/" + to_string(date[2]);


            curl = curl_easy_init(); // инициализация объекта CURL

            if (curl) {
                xmlFile.open("output.xml"); // открываем файл для записи в бинарном режиме

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // устанавливаем URL для загрузки
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &xmlFile); // указываем файл для записи полученных данных
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // устанавливаем функцию обратного вызова для записи данных

                res = curl_easy_perform(curl); // выполняем запрос

                if (res != CURLE_OK) {
                    std::cerr << "DFJSMHDBFHSD: " << curl_easy_strerror(res) << std::endl;
                }

                curl_easy_cleanup(curl); // освобождаем ресурсы
                xmlFile.close(); // закрываем файл

                pugi::xml_document doc;
                pugi::xml_parse_result result = doc.load_file("output.xml");

                // Извлекаем дату из узла "ValCurs"
                std::string date = doc.child("ValCurs").attribute("Date").as_string();

                string outputInfo; // Переменная для хранения конечной информации

                for (pugi::xml_node valute : doc.child("ValCurs").children("Valute")) {
                    std::string charCode = valute.child_value("CharCode");
                    if (charCode == param_vname) {
                        std::string value_str = valute.child_value("Value");
                        std::string nominal = valute.child_value("Nominal");

                        // Проверяем, если параметр vnom не равен "-", то пересчитываем значение value и меняем nominal
                        if (param_vnom != "-") {
                            string nom_for_one = valute.child_value("Value");
                            double nom_for_one_double = stringToDouble(nom_for_one);
                            double param_vnom_double = stringToDouble(param_vnom);
                            double value = nom_for_one_double * param_vnom_double;
                            value_str = to_string(value);
                            nominal = param_vnom;
                        }

                        outputInfo = param_vname + ' ' + nominal + ' ' + date + ' ' + value_str; // Сохраняем значение и номинал в outputInfo
                        answers.push_back(outputInfo);
                        break;  // Выход из цикла после найденной валюты
                    }
                }

                
            }
        }
        if (param_filename != "-") {
            std::ofstream outputFile(param_filename);
            for (auto i : answers) {
                outputFile << i << '\n';
            }
            outputFile.close();
        }
        else {
            for (auto i : answers) {
                cout << i << '\n';
            }
        }

    }
    return 0;
}