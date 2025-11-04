FROM node:alpine

RUN echo 'http://dl-cdn.alpinelinux.org/alpine/v3.6/main' >> /etc/apk/repositories
RUN echo 'http://dl-cdn.alpinelinux.org/alpine/v3.6/community' >> /etc/apk/repositories
RUN apk add --update --no-cache supervisor mariadb-client curl

RUN mkdir -p /app/logs
RUN touch /app/logs/requests.log

WORKDIR /app
COPY challenge .

RUN yarn

COPY config/supervisord.conf /etc/supervisord.conf

EXPOSE 7331

COPY --chown=root entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
