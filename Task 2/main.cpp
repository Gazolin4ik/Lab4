#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

using namespace std;

struct SessionResult {
    char semester; // семестр
    string subject; //предмет
    int grade; // оценка
};

struct Student {
    string fio;//имя
    string group; //группа
    vector<SessionResult> results; // результат сессии
};

// Генератор случайного имени
string generateRandomName() {
    vector<string> firstNames = { "Илья", "Петр", "Егор", "Даша", "Дима", "Настя" }; //всевозможные имена для генерации
    vector<string> lastNames = { "Иванов(-а)", "Петров(-а)", "Сидоров(-а)", "Кузнецов(-а)", "Смирнов(-а)", "Попов(-а)" }; //всевозможные фамилии для генерации

    random_device rd; // генератор для имени
    mt19937 gen(rd());
    uniform_int_distribution<> distFirst(0, firstNames.size() - 1); // выбираем из всевозможных имен
    uniform_int_distribution<> distLast(0, lastNames.size() - 1); // выбираем всевозможные фамилии

    return firstNames[distFirst(gen)] + " " + lastNames[distLast(gen)]; // объединяем их
}

// Генерация данных о студентах с полным заполнением оценками
vector<Student> generateStudents(int count) {
    vector<Student> students; // вектор для студентов 
    vector<string> groups = { "А", "Б", "В", "Г" };
    vector<string> subjects = { "Физика", "Английский", "Программирование", "Электротехника"};
    vector<char> semesters = { 'A', 'B', 'C', 'D' }; // Семестры обозначены символами

    random_device rd; 
    mt19937 gen(rd());
    uniform_int_distribution<> distGroup(0, groups.size() - 1); // генератор для группы
    uniform_int_distribution<> distGrade(2, 5); // генератор для оценки

    for (int i = 0; i < count; ++i) {
        Student student; // создаем студента
        student.fio = generateRandomName(); // заполняем имя
        student.group = groups[distGroup(gen)]; //рандомно выбираем группу студенту

        // Заполнение результатов сессий для всех предметов и всех семестров
        for (char semester : semesters) { // проходимся по семестрам всем
            for (const string& subject : subjects) { // проходимся по предмету
                SessionResult result; // для каждого студента создаем результат сессии
                result.semester = semester;  // записываем семестр
                result.subject = subject;    // записываем
                result.grade = distGrade(gen); // рандомно ставим оценку студенту
                student.results.push_back(result); // записываем результат сессии в общую успевемость студента
            }
        }

        students.push_back(student); // добавляем студента
    }

    return students;
}

// Функция для вычисления средней успеваемости (однопоточно)
double calculateAverageGrade(const vector<Student>& students, const string& targetGroup, char targetSemester) {
    int sumGrades = 0;// сумма нужных оценок
    int countGrades = 0; //количество оценок
    for (const auto& student : students) { // проходимся по всем студентам
        if (student.group == targetGroup) { // если совпала нужная нам группа
            for (const auto& result : student.results) { // проходимся по всем результатам сессий
                if (result.semester == targetSemester) { // если совпал семестр 
                    sumGrades += result.grade; // записываем оценку в общую сумму
                    countGrades++; // увеличиваем кол-во оценок
                }
            }
        }
    }
    return countGrades == 0 ? 0.0 : static_cast<double>(sumGrades) / countGrades; // если оценок 0,то возвращаем 0.0 ,иначе   приводим к типу double 
}

// Многопоточная версия вычисления средней успеваемости
double calculateAverageGradeParallel(const vector<Student>& students, const string& targetGroup, char targetSemester, int threadCount) {
    int sumGrades = 0; // сумма оценок
    int countGrades = 0; //кол-во оценок
    mutex mutex; // мьютекс

    auto processPart = [&](int start, int end) { // лямбда функции
        int localSum = 0; // сумма для каждого потока
        int localCount = 0; // кол-во оценок для каждого потока 
        for (int i = start; i < end; ++i) { // проходимся по заданным границам
            if (students[i].group == targetGroup) { // если нашли нужную группу
                for (const auto& result : students[i].results) { //проходися по результатам сессиии
                    if (result.semester == targetSemester) { //если нашли нужный семестр
                        localSum += result.grade; // добавляем в сумму оценку
                        localCount++; // увеличиваем кол-во оценок
                    }
                }
            }
        }
        lock_guard<std::mutex> lock(mutex);
        sumGrades += localSum;
        countGrades += localCount;
    };

    vector<thread> threads; // вектор всех потоков
    int partSize = students.size() / threadCount; // часть студентов, которую определнный поток должен обработать
    for (int i = 0; i < threadCount; ++i) {
        int start = i * partSize; // начальный элемент
        int end = (i == threadCount - 1) ? students.size() : start + partSize; //конечный элемент
        threads.emplace_back(processPart, start, end); // создаем поток и добавляем его в вектор
    }

    for (auto& thread : threads) {
        thread.join();// ожидаем чтобы все потоки закончили свою работу
    }

    return countGrades == 0 ? 0.0 : static_cast<double>(sumGrades) / countGrades; //возвращаем результат
}

// Функция для вывода информации о всех студентах
void printStudents(const vector<Student>& students) {
    for (const auto& student : students) { // проходимся по всем студентам
        cout << "ФИО: " << student.fio << "\nГруппа: " << student.group << "\nСессии:\n"; //выводим данные о студенте
        for (const auto& result : student.results) { //выводим данные о его успеваемости
            cout << "  Семестр: " << result.semester
                << ", Дисциплина: " << result.subject
                << ", Оценка: " << result.grade << '\n';
        }
        cout << "----------------------------------\n";
    }
}

int main() {
    int studentCount = 10;   // Задаем количество студентов для генерации
    int threadCount = 4;    // Количество потоков
    string group = "Г"; // Группа для анализа
    char semester = 'C';     // Семестр для анализа

    // Генерация студентов
    auto students = generateStudents(studentCount);

    // Вывод всех сгенерированных студентов
    printStudents(students);

    // Однопоточное вычисление
    auto startSingle = chrono::high_resolution_clock::now(); // таймер для начала
    double avgSingle = calculateAverageGrade(students, group, semester); // рассчет успеваемости
    auto endSingle = chrono::high_resolution_clock::now(); // таймер для конца
    chrono::duration<double> elapsedSingle = endSingle - startSingle; // находим разницу таймером в секундах

    // Многопоточное вычисление
    auto startMulti = chrono::high_resolution_clock::now(); //таймер для начала
    double avgMulti = calculateAverageGradeParallel(students, group, semester, threadCount); //рассчет успеваемости
    auto endMulti = chrono::high_resolution_clock::now();//таймер для конца
    chrono::duration<double> elapsedMulti = endMulti - startMulti; // находим разницу таймеров в секундах

    // Вывод результатов
    cout << "Средняя успеваемость (однопоточно): " << avgSingle << " за " << elapsedSingle.count() << " секунд\n";
    cout << "Средняя успеваемость (многопоточно): " << avgMulti << " за " << elapsedMulti.count() << " секунд\n";

    return 0;
}
