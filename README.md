#

Documentation for project folders in `/docs`

## Docker

### Base Image

Debian + all Dependencies, can be used for CLion Docker Development environment

```sh
docker build -f ./dockerfiles/debian_base.dockerfile -t hutii/drdb:base .
```

### Debug Image

```sh
docker build -f ./dockerfiles/drdb_debug.dockerfile --target builder -t hutii/drdb:debug .
```

### Debug Image Slim

```sh
docker build -f ./dockerfiles/drdb_debug.dockerfile -t hutii/drdb:latest .
```

### Release Image Slim

```sh
docker build -f ./dockerfiles/drdb_release.dockerfile -t hutii/drdb:release .
```

## Docker Compose

```sh
docker compose -f docker-compose.2servers-1client.yml up --build
```

```sh
docker compose -f docker-compose.4servers-4clients.yml up --build
```

```sh
docker compose -f docker-compose.4servers-no-clients.yml up --build
```

Running manually works, but Docker Compose gets a Segfault

```sh
docker compose -f docker-compose.jni-1server-1client.yml up --build
```

## DockerHub & GitHub Actions

https://hub.docker.com/r/hutii/drdb/

Any Version of the Project that gets pushed inside the `releases/001` branch will be compiled via GitHub Actions and pushed to dockerhub. This Image will be based on `./dockerfiles/drdb_debug.dockerfile`.

Similary any Version of the Project that gets pushed inside the `releases/002` branch will be compiled via GitHub Actions and pushed to dockerhub. This Image will be based on `./dockerfiles/drdb_release.dockerfile`.

The difference is that the release Image uses a cmake release build, instead of the default debug build.

## Singularity

https://cloud.sylabs.io/library/hutan/drdb/drdb

### Remote Building

https://cloud.sylabs.io/builder

Using the Remote Builder one can easily create a Sylabs Container from the Docker Image on DockerHub.

The build Recipe is in `./singularity/recipe`

### Local Build with buildx

```sh
docker buildx build --platform linux/amd64,linux/arm64 -f ./dockerfiles/drdb.dockerfile -t hutii/drdb:latest .

docker push hutii/drdb:latest
```
