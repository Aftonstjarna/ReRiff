FROM docker.io/library/python:3.13-slim-bookworm

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    RIFFAPI_DATA_DIR=/data
WORKDIR /app
COPY requirements-container.txt /app/requirements-container.txt
RUN python -m pip install --no-cache-dir -r requirements-container.txt \
    && groupadd --gid 10001 riffapi \
    && useradd --uid 10001 --gid 10001 --no-create-home riffapi \
    && mkdir -p /data \
    && chown 10001:10001 /data
COPY riffapi-local.py admin.html LICENSE /app/
COPY scripts/run-container.py scripts/container-healthcheck.py /app/scripts/

USER 10001:10001
VOLUME ["/data"]
EXPOSE 5144/tcp
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD ["python", "/app/scripts/container-healthcheck.py"]
CMD ["python", "/app/scripts/run-container.py"]
