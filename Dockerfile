FROM php:5.6-apache

# Install system dependencies
RUN apt-get update && apt-get install -y libfreetype6 libjpeg62-turbo libpng12-0 --no-install-recommends && rm -rf /var/lib/apt/lists/*
RUN buildDeps=" \
		libfreetype6-dev \
		libjpeg-dev \
		libldap2-dev \
		libmcrypt-dev \
		libpng12-dev \
		zlib1g-dev \
	"; \
	apt-get update && apt-get install -y $buildDeps --no-install-recommends && rm -rf /var/lib/apt/lists/*

# Install needed PHP extensions
RUN docker-php-ext-install gd mbstring mysql mysqli opcache pdo pdo_mysql zip

# Enable Apache's mod_rewrite
RUN cd /etc/apache2/mods-enabled && ln -s ../mods-available/rewrite.load rewrite.load

# Copy php fusion to apache web root
COPY . /var/www/html/
