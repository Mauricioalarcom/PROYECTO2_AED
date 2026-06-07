#ifndef CHILD_MAP_H
#define CHILD_MAP_H

#include <cstddef>
#include <utility>
#include <vector>

// ===========================================================================
// ChildMap: tabla hash (caracter -> indice de nodo) IMPLEMENTADA DESDE CERO.
//
// Reemplaza a std::unordered_map en la adyacencia de hijos del Suffix Tree para
// no usar contenedores asociativos de la STL (regla 4 del enunciado: no se
// permite std::map / std::set / std::unordered_map ni equivalentes).
//
// Tecnica: direccionamiento abierto con sondeo lineal y capacidad potencia de
// dos (la mascara reemplaza al modulo). El unico almacenamiento es un
// std::vector<Entry> (contenedor basico permitido); el hashing, el sondeo, el
// control del factor de carga y el rehash son propios.
//
// Operaciones usadas por el arbol: get (O(1) promedio), set (O(1) amortizado),
// empty, size e iteracion (forEach).
// ===========================================================================
class ChildMap {
public:
    struct Entry {
        unsigned char key  = 0;
        int           val  = -1;
        bool          used = false;
    };

    int  size()  const { return count_; }
    bool empty() const { return count_ == 0; }

    // Valor asociado a 'k', o -1 si no esta.
    int get(unsigned char k) const {
        if (cap_ == 0) return -1;
        const std::size_t mask = cap_ - 1;
        std::size_t i = hash(k) & mask;
        while (slots_[i].used) {
            if (slots_[i].key == k) return slots_[i].val;
            i = (i + 1) & mask;                 // sondeo lineal
        }
        return -1;
    }

    bool contains(unsigned char k) const { return get(k) >= 0; }

    // Inserta o actualiza (k -> v).
    void set(unsigned char k, int v) {
        if (cap_ == 0)
            rehash(kInitialCap);
        else if ((count_ + 1) * 10 >= static_cast<int>(cap_) * 7)   // carga > 0.7
            rehash(cap_ * 2);
        insertRaw(slots_, cap_, k, v, &count_);
    }

    // Itera las parejas presentes. F debe aceptar (unsigned char key, int val).
    template <class F>
    void forEach(F&& f) const {
        for (const Entry& e : slots_)
            if (e.used) f(e.key, e.val);
    }

private:
    static constexpr std::size_t kInitialCap = 8;   // potencia de dos

    std::vector<Entry> slots_;
    std::size_t        cap_   = 0;                   // siempre potencia de dos
    int                count_ = 0;

    static std::size_t hash(unsigned char k) {
        // Mezcla simple; con sondeo lineal y dominio de 256 claves es suficiente.
        return static_cast<std::size_t>(k) * 131u + 7u;
    }

    // Inserta en 'slots' (capacidad 'cap'); si 'count' no es nulo y la clave es
    // nueva, lo incrementa.
    static void insertRaw(std::vector<Entry>& slots, std::size_t cap,
                          unsigned char k, int v, int* count) {
        const std::size_t mask = cap - 1;
        std::size_t i = hash(k) & mask;
        while (slots[i].used) {
            if (slots[i].key == k) { slots[i].val = v; return; }   // actualizar
            i = (i + 1) & mask;
        }
        slots[i].used = true;
        slots[i].key  = k;
        slots[i].val  = v;
        if (count) ++(*count);
    }

    void rehash(std::size_t newCap) {
        std::vector<Entry> ns(newCap);              // todos used=false
        for (const Entry& e : slots_)
            if (e.used) insertRaw(ns, newCap, e.key, e.val, nullptr);
        slots_ = std::move(ns);
        cap_   = newCap;
    }
};

#endif // CHILD_MAP_H
