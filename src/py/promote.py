def promote_token(t):
    try:
        return int(t)
    except ValueError:
        pass

    try:
        return float(t)
    except ValueError:
        pass

    return t


