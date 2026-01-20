#!/bin/bash
# Script interattivo per configurare GitLab Container Registry

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
RED='\033[0;31m'
NC='\033[0m'

clear

echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║                                                          ║${NC}"
echo -e "${BLUE}║       GitLab Container Registry - Setup Wizard          ║${NC}"
echo -e "${BLUE}║                                                          ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

# Step 1: Raccogli informazioni
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Step 1: Informazioni GitLab${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

echo "Hai già un progetto GitLab?"
echo "  Se no, crealo ora su: https://gitlab.com/projects/new"
echo ""

read -p "GitLab project path (es. username/grpc-orchestrator): " PROJECT_PATH

if [ -z "$PROJECT_PATH" ]; then
    echo -e "${RED}✗ Project path richiesto!${NC}"
    exit 1
fi

echo ""
echo "Hai già un Access Token?"
echo "  Se no, crealo su: https://gitlab.com/-/profile/personal_access_tokens"
echo "  Scope richiesti: read_registry, write_registry"
echo ""

read -p "GitLab username: " GITLAB_USER

if [ -z "$GITLAB_USER" ]; then
    echo -e "${RED}✗ Username richiesto!${NC}"
    exit 1
fi

read -sp "GitLab access token (nascosto): " GITLAB_TOKEN
echo ""

if [ -z "$GITLAB_TOKEN" ]; then
    echo -e "${RED}✗ Token richiesto!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}✓ Informazioni raccolte${NC}"

# Step 2: Test login
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Step 2: Test Login${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

echo "Testing login a registry.gitlab.com..."
echo "$GITLAB_TOKEN" | docker login registry.gitlab.com -u "$GITLAB_USER" --password-stdin

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ Login riuscito!${NC}"
else
    echo ""
    echo -e "${RED}✗ Login fallito! Verifica username e token.${NC}"
    exit 1
fi

# Step 3: Backup configs
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Step 3: Backup Configurazioni${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

if [ -f "deploy/builder_config.yaml" ]; then
    cp deploy/builder_config.yaml deploy/builder_config.yaml.backup
    echo -e "${GREEN}✓ Backup: deploy/builder_config.yaml.backup${NC}"
fi

if [ -f "deploy/deployment_config.yaml" ]; then
    cp deploy/deployment_config.yaml deploy/deployment_config.yaml.backup
    echo -e "${GREEN}✓ Backup: deploy/deployment_config.yaml.backup${NC}"
fi

# Step 4: Aggiorna configurazioni
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Step 4: Aggiorna Configurazioni${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Update builder_config.yaml
if [ -f "deploy/builder_config.yaml" ]; then
    echo "Aggiornando deploy/builder_config.yaml..."
    
    # Abilita GitLab
    sed -i 's/enabled: false/enabled: true/' deploy/builder_config.yaml
    
    # Aggiorna project_path
    sed -i "s|project_path:.*|project_path: \"$PROJECT_PATH\"|" deploy/builder_config.yaml
    
    # Aggiorna username
    sed -i "s|username:.*|username: \"$GITLAB_USER\"|" deploy/builder_config.yaml
    
    # Aggiorna token
    sed -i "s|access_token:.*|access_token: \"$GITLAB_TOKEN\"|" deploy/builder_config.yaml
    
    echo -e "${GREEN}✓ builder_config.yaml aggiornato${NC}"
fi

# Update deployment_config.yaml
if [ -f "deploy/deployment_config.yaml" ]; then
    echo "Aggiornando deploy/deployment_config.yaml..."
    
    # Trova la sezione gitlab e abilita
    sed -i '/gitlab:/,/use_ci_token:/ {
        s/enabled: false/enabled: true/
    }' deploy/deployment_config.yaml
    
    # Aggiorna project_path nella sezione gitlab
    sed -i "/gitlab:/,/use_ci_token:/ {
        s|project_path:.*|project_path: \"$PROJECT_PATH\"|
    }" deploy/deployment_config.yaml
    
    # Aggiorna username
    sed -i "/gitlab:/,/use_ci_token:/ {
        s|username:.*|username: \"$GITLAB_USER\"|
    }" deploy/deployment_config.yaml
    
    # Aggiorna token
    sed -i "/gitlab:/,/use_ci_token:/ {
        s|access_token:.*|access_token: \"$GITLAB_TOKEN\"|
    }" deploy/deployment_config.yaml
    
    echo -e "${GREEN}✓ deployment_config.yaml aggiornato${NC}"
fi

# Step 5: Aggiungi al .gitignore
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Step 5: Sicurezza${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

echo "Vuoi aggiungere i config al .gitignore per evitare di committare il token? (y/n)"
read -p "" -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    if ! grep -q "deploy/builder_config.yaml" .gitignore 2>/dev/null; then
        echo "deploy/builder_config.yaml" >> .gitignore
        echo -e "${GREEN}✓ Aggiunto deploy/builder_config.yaml a .gitignore${NC}"
    fi
    
    if ! grep -q "deploy/deployment_config.yaml" .gitignore 2>/dev/null; then
        echo "deploy/deployment_config.yaml" >> .gitignore
        echo -e "${GREEN}✓ Aggiunto deploy/deployment_config.yaml a .gitignore${NC}"
    fi
fi

# Step 6: Verifica
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${YELLOW}Step 6: Verifica Configurazione${NC}"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

echo "Configurazione GitLab:"
echo "  Registry: registry.gitlab.com"
echo "  Project: $PROJECT_PATH"
echo "  Username: $GITLAB_USER"
echo "  Token: ${GITLAB_TOKEN:0:8}..."
echo ""

# Step 7: Prossimi passi
echo -e "${BLUE}╔══════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║               SETUP COMPLETATO CON SUCCESSO!             ║${NC}"
echo -e "${BLUE}╚══════════════════════════════════════════════════════════╝${NC}"
echo ""

echo -e "${GREEN}Prossimi Passi:${NC}"
echo ""
echo "1. Build e push immagini su GitLab:"
echo "   ${CYAN}./build/bin/builder_manager_main build${NC}"
echo ""
echo "2. Verifica immagini su GitLab:"
echo "   ${CYAN}https://gitlab.com/$PROJECT_PATH/-/packages${NC}"
echo ""
echo "3. Deploy usando immagini da GitLab:"
echo "   ${CYAN}sudo ./build/bin/deploy_manager_main deploy${NC}"
echo ""
echo "4. (Opzionale) Test pull manuale:"
echo "   ${CYAN}sudo ./build/bin/deploy_manager_main pull${NC}"
echo ""

echo -e "${YELLOW}Note:${NC}"
echo "  - Backup salvati in deploy/*.backup"
echo "  - Per tornare a locale: gitlab.enabled: false nei config"
echo "  - Guida completa: GITLAB_SETUP.md"
echo ""

echo -e "${CYAN}Vuoi procedere con build e push ora? (y/n)${NC}"
read -p "" -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "Building e pushing immagini..."
    ./build/bin/builder_manager_main build --config deploy/builder_config.yaml
    
    if [ $? -eq 0 ]; then
        echo ""
        echo -e "${GREEN}✓ Immagini buildate e pushate su GitLab!${NC}"
        echo ""
        echo "Verifica su: https://gitlab.com/$PROJECT_PATH/-/packages"
        echo ""
        echo "Per deployare:"
        echo "  sudo ./build/bin/deploy_manager_main deploy"
    else
        echo ""
        echo -e "${RED}✗ Build/push fallito. Controlla i logs.${NC}"
    fi
fi

echo ""
echo -e "${GREEN}Setup GitLab completato! 🎉${NC}"
echo ""
