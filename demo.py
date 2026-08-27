class Vector_3:
    x: float = 0
    y: float = 0
    z: float = 0

    def __init__(self, x: float, y: float, z: float):
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return f"{{{self.x},{self.y},{self.z}}}"


def vector3_add(a: Vector_3, b: Vector_3) -> Vector_3:
    return Vector_3(a.x + b.x, a.y + b.y, a.z + b.z)


def vector3_sub(a: Vector_3, b: Vector_3) -> Vector_3:
    return Vector_3(a.x - b.x, a.y - b.y, a.z - b.z)


def vector3_scale(v: Vector_3, s: float) -> Vector_3:
    return Vector_3(v.x * s, v.y * s, v.z * s)


def vector3_dot(a: Vector_3, b: Vector_3) -> float:
    return a.x * b.x + a.y * b.y + a.z * b.z


def vector3_cross(a: Vector_3, b: Vector_3) -> Vector_3:
    return Vector_3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    )


def vector3_length(v: Vector_3) -> float:
    return pow(v.x * v.x + v.y * v.y + v.z * v.z, 0.5)


def vector3_normalize(v: Vector_3) -> Vector_3:
    length = vector3_length(v)
    if length == 0:
        return Vector_3(0, 0, 0)
    return vector3_scale(v, 1.0 / length)


def vector3_lerp(a: Vector_3, b: Vector_3, t: float) -> Vector_3:
    return vector3_add(a, vector3_scale(vector3_sub(b, a), t))


def vector3_reflect(d: Vector_3, n: Vector_3) -> Vector_3:
    return vector3_sub(d, vector3_scale(n, 2.0 * vector3_dot(d, n)))


if __name__ == "__main__":
    v1 = Vector_3(1, 2, 3)
    print(vector3_normalize(v1))
    a = Vector_3(1, 0, 0)
    b = Vector_3(0, 1, 0)
    print(vector3_dot(a, b), vector3_cross(a, b))
