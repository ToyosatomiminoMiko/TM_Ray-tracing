class vec3:
    x: float = 0
    y: float = 0
    z: float = 0

    def __init__(self, x: float, y: float, z: float):
        self.x = x
        self.y = y
        self.z = z

    def __str__(self):
        return f"{{{self.x},{self.y},{self.z}}}"


def vec3_add(a: vec3, b: vec3) -> vec3:
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z)


def vec3_sub(a: vec3, b: vec3) -> vec3:
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z)


def vec3_scale(v: vec3, s: float) -> vec3:
    return vec3(v.x * s, v.y * s, v.z * s)


def vec3_dot(a: vec3, b: vec3) -> float:
    return a.x * b.x + a.y * b.y + a.z * b.z


def vec3_cross(a: vec3, b: vec3) -> vec3:
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    )


def vec3_length(v: vec3) -> float:
    return pow(v.x * v.x + v.y * v.y + v.z * v.z, 0.5)


def vec3_normalize(v: vec3) -> vec3:
    length = vec3_length(v)
    if length == 0:
        return vec3(0, 0, 0)
    return vec3_scale(v, 1.0 / length)


def vec3_lerp(a: vec3, b: vec3, t: float) -> vec3:
    return vec3_add(a, vec3_scale(vec3_sub(b, a), t))


def vec3_reflect(d: vec3, n: vec3) -> vec3:
    return vec3_sub(d, vec3_scale(n, 2.0 * vec3_dot(d, n)))


if __name__ == "__main__":
    v1 = vec3(1, 2, 3)
    print(vec3_normalize(v1))
    a = vec3(1, 0, 0)
    b = vec3(0, 1, 0)
    print(vec3_dot(a, b), vec3_cross(a, b))
