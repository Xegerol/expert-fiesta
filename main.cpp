#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

// Directions: 0=North, 1=East, 2=South, 3=West
// dx/dy for each direction (row, col)
static const int DR[] = {-1, 0, 1, 0};
static const int DC[] = { 0, 1, 0,-1};

class Animal {
protected:
    int x, y;   // row, col
    int dir;    // direction 0-3
    int stab;   // stability
    int age;
public:
    Animal() : x(0), y(0), dir(0), stab(1), age(0) {}
    Animal(int x, int y, int dir, int stab)
        : x(x), y(y), dir(dir), stab(stab), age(0) {}

    int get_x()   const { return x; }
    int get_y()   const { return y; }
    int get_dir() const { return dir; }
    int get_s()   const { return stab; }
    int get_age() const { return age; }

    void set_x(int v) { x = v; }
    void set_y(int v) { y = v; }

    // Move by 'steps' cells, wrapping on NxM grid
    void move(int N, int M, int steps) {
        x = ((x + DR[dir] * steps) % N + N) % N;
        y = ((y + DC[dir] * steps) % M + M) % M;
    }

    // Rotate direction clockwise
    void changeD() {
        dir = (dir + 1) % 4;
    }

    void age1() { ++age; }
};

class Rabbit : public Animal {
public:
    Rabbit() : Animal() {}
    Rabbit(int x, int y, int dir, int stab) : Animal(x, y, dir, stab) {}
};

class Fox : public Animal {
    int food; // rabbits eaten since last birth
public:
    Fox() : Animal(), food(0) {}
    Fox(int x, int y, int dir, int stab) : Animal(x, y, dir, stab), food(0) {}
    Fox(int x, int y, int dir, int stab, int age_val) : Animal(x, y, dir, stab), food(0) {
        age = age_val;
    }

    int get_food() const { return food; }
    void set_food(int v) { food = v; }
};

class Model {
    int N, M, K;
    int R, F;
    std::vector<Rabbit> masR;
    std::vector<Fox>    masF;
    int **mas; // grid: positive = rabbits, negative = foxes

    void rebuild_grid() {
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < M; ++j)
                mas[i][j] = 0;
        for (auto &r : masR) ++mas[r.get_x()][r.get_y()];
        for (auto &f : masF) --mas[f.get_x()][f.get_y()];
    }

public:
    Model(int n, int m, int k) : N(n), M(m), K(k), R(0), F(0) {
        mas = new int*[N];
        for (int i = 0; i < N; ++i) {
            mas[i] = new int[M];
            for (int j = 0; j < M; ++j) mas[i][j] = 0;
        }
    }

    Model(Model const &that) : N(that.N), M(that.M), K(that.K),
                               R(that.R), F(that.F),
                               masR(that.masR), masF(that.masF),
                               mas(new int*[that.N]) {
        for (int i = 0; i < N; ++i) {
            mas[i] = new int[M];
            for (int j = 0; j < M; ++j) mas[i][j] = that.mas[i][j];
        }
    }

    ~Model() {
        for (int i = 0; i < N; ++i) delete[] mas[i];
        delete[] mas;
    }

    void addR(int x, int y, int dir, int s) {
        masR.push_back(Rabbit(x, y, dir, s));
        ++mas[x][y];
        ++R;
    }

    void addF(int x, int y, int dir, int s) {
        masF.push_back(Fox(x, y, dir, s));
        --mas[x][y];
        ++F;
    }

    void step() {
        // 1. Movement
        // Rabbits move 1 cell
        for (int i = 0; i < R; ++i) {
            --mas[masR[i].get_x()][masR[i].get_y()];
            masR[i].move(N, M, 1);
            ++mas[masR[i].get_x()][masR[i].get_y()];
        }
        // Foxes move 2 cells
        for (int i = 0; i < F; ++i) {
            ++mas[masF[i].get_x()][masF[i].get_y()]; // un-mark fox (negative)
            masF[i].move(N, M, 2);
            --mas[masF[i].get_x()][masF[i].get_y()]; // re-mark fox
        }

        // After movement, change direction if stability requires it
        // (done after feeding per spec — but spec says "after movement change dir if stability requires")
        // Actually re-reading: "After movement animal changes direction if stability requires it"
        // We apply direction change after movement but before feeding.
        for (int i = 0; i < R; ++i) {
            // stability: the animal changes direction every 'stab' moves
            // age hasn't been incremented yet; use (age+1) as move count after this step
            // Simpler interpretation: change direction when (age+1) % stab == 0
            if ((masR[i].get_age() + 1) % masR[i].get_s() == 0)
                masR[i].changeD();
        }
        for (int i = 0; i < F; ++i) {
            if ((masF[i].get_age() + 1) % masF[i].get_s() == 0)
                masF[i].changeD();
        }

        // 2. Feeding: foxes eat rabbits on same cell
        // Older foxes eat first — masF is ordered by seniority (earlier index = older/senior)
        for (int fi = 0; fi < F; ++fi) {
            int fx = masF[fi].get_x(), fy = masF[fi].get_y();
            for (int ri = 0; ri < R; ) {
                if (masR[ri].get_x() == fx && masR[ri].get_y() == fy) {
                    --mas[fx][fy]; // remove rabbit from grid count
                    masR.erase(masR.cbegin() + ri);
                    --R;
                    masF[fi].set_food(masF[fi].get_food() + 1);
                    // first fox eats ALL rabbits on that cell
                } else {
                    ++ri;
                }
            }
        }

        // 3. Aging
        for (int i = 0; i < R; ++i) masR[i].age1();
        for (int i = 0; i < F; ++i) masF[i].age1();

        // 4. Reproduction
        // Rabbits breed at age 5 and 10
        int rSize = R;
        for (int j = 0; j < rSize; ++j) {
            if (masR[j].get_age() == 5 || masR[j].get_age() == 10) {
                masR.push_back(Rabbit(masR[j].get_x(), masR[j].get_y(),
                                      masR[j].get_dir(), masR[j].get_s()));
                ++mas[masR[j].get_x()][masR[j].get_y()];
                ++R;
            }
        }
        // Foxes breed after eating >= 2 rabbits
        int fSize = F;
        for (int j = 0; j < fSize; ++j) {
            if (masF[j].get_food() >= 2) {
                masF.push_back(Fox(masF[j].get_x(), masF[j].get_y(),
                                   masF[j].get_dir(), masF[j].get_s(), 0));
                --mas[masF[j].get_x()][masF[j].get_y()];
                ++F;
                masF[j].set_food(0);
            }
        }

        // 5. Death
        // Rabbits die at age 10
        for (int j = 0; j < R; ) {
            if (masR[j].get_age() == 10) {
                --mas[masR[j].get_x()][masR[j].get_y()];
                masR.erase(masR.cbegin() + j);
                --R;
            } else {
                ++j;
            }
        }
        // Foxes die at age 15
        for (int j = 0; j < F; ) {
            if (masF[j].get_age() == 15) {
                ++mas[masF[j].get_x()][masF[j].get_y()];
                masF.erase(masF.cbegin() + j);
                --F;
            } else {
                ++j;
            }
        }
    }

    void write(std::ostream &out) const {
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < M; ++j) {
                int v = mas[i][j];
                if (v == 0)
                    out << '*';
                else if (v > 0)
                    out << v;
                else
                    out << v; // prints negative number like -1
            }
            out << '\n';
        }
    }

    void run() {
        for (int i = 0; i < K; ++i) step();
    }
};

int main() {
    std::ifstream in("input.txt");
    std::ofstream out("output.txt");

    int N, M, K;
    in >> N >> M >> K;

    int numR, numF;
    in >> numR >> numF;

    Model model(N, M, K);

    for (int i = 0; i < numR; ++i) {
        int x, y, d, s;
        in >> x >> y >> d >> s;
        model.addR(x, y, d, s);
    }
    for (int i = 0; i < numF; ++i) {
        int x, y, d, s;
        in >> x >> y >> d >> s;
        model.addF(x, y, d, s);
    }

    model.run();
    model.write(out);

    return 0;
}
