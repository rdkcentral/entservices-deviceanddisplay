#!/bin/bash

# Export your environment variables
export GIT_USERNAME=akrish905_comcast
export GIT_PASSWORD=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
# Use envsubst to replace placeholders in peru.yaml
export GERRIT_USERNAME=kdarma930
export GERRIT_PASSWORD=5gDwn5jpTI9AKLsaZ9du6arlR1JMZqMXu26Ikynn1w

echo "GIT_USERNAME: $GIT_USERNAME"
echo "GIT_PASSWORD: $GIT_PASSWORD"

envsubst < peru.yaml > peru_temp.yaml
echo "Substituted peru.yaml:"
cat peru_temp.yaml
# Move the substituted file to replace the original peru.yaml
chmod 777 peru_temp.yaml
rm -rf peru.yaml
mv peru_temp.yaml peru.yaml
cat peru.yaml

chmod 644 peru.yaml
# Run peru commands
echo "Running peru sync..."
#peru sync --no-cache -v

#peru sync && peru sync --no-cache
#peru sync --no-cache

# Clean up
#rm peru.yaml
